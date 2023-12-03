#include <kmod.h>
#include <mm/vmm.h>
#include <mm/addr.h>
#include <stdlib.h>
#include <hal/fbuf.h>
#include <hal/timer.h>
#include "bios.h"

/* default (fallback) resolution */
#define VBE_FALLBACK_WIDTH                      640
#define VBE_FALLBACK_HEIGHT                     480

typedef struct {
    uint16_t mode;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint16_t pitch;
    uintptr_t framebuffer_ptr;
} vbe_mode_t;

static vbe_mode_t* vbe_modes = NULL; // list of video modes
static size_t vbe_modes_len = 0;
static vbe_mode_t* vbe_mode_current = NULL; // current video mode
static void* vbe_framebuffer = NULL;

static fbuf_t vbe_fbuf_impl;

static vbe_mode_t* vbe_find_mode(size_t width, size_t height, size_t bpp) {
    size_t idx = (size_t)-1; // index of most suitable mode
    for(size_t i = 0; i < vbe_modes_len; i++) {
        if(vbe_modes[i].width == width && vbe_modes[i].height == height && (!bpp || vbe_modes[i].bpp == bpp)) {
            if(idx == (size_t)-1) idx = i;
            if(bpp) { // chosen one right here
                idx = i;
                break;
            } else if(vbe_modes[i].bpp > vbe_modes[idx].bpp) idx = i;
        }
    }
    
    if(idx == (size_t)-1) {
        if(bpp) kerror("cannot find any modes of resolution %ux%ux%u", width, height, bpp);
        else kerror("cannot find any modes of resolution %ux%u", width, height);
        return NULL;
    } else return &vbe_modes[idx];
}

int32_t kmod_init() {
    kinfo("Generic VESA BIOS Extensions driver for SysX");

    vmm_pgmap(vmm_current, VBE_DATA_PADDR, VBE_DATA_VADDR, VMM_FLAGS_PRESENT | VMM_FLAGS_RW);

    /* read controller info */
    kdebug("reading controller info");
    vbe_ctrlinfo_t* ctrlinfo = vbe_get_ctrlinfo();
    if(ctrlinfo == NULL) {
        kerror("VBE is not supported");
        return -1;
    } else kinfo("video card supports VBE %u.%u, VRAM size: %uK", ctrlinfo->ver_maj, ctrlinfo->ver_min, ctrlinfo->memory * 64);

    /* parse video modes list */
    kdebug("discovering video modes");
    uint16_t* modes = (uint16_t*) VBE_LINEAR_ADDR(ctrlinfo->modes_seg, ctrlinfo->modes_off);
    kinfo("VBE specifies video mode list at 0x%x", modes);
    bool map = false;
    if((uintptr_t) modes <= VBE_DATA_PADDR || (uintptr_t) modes >= VBE_DATA_PADDR + sizeof(vbe_ctrlinfo_t)) {
        /* map video modes to memory before proceeding */
        map = true; // so we'll remember to unmap it later
        uintptr_t offset = (uintptr_t) modes & 0xFFF;
        uintptr_t vaddr = vmm_first_free(vmm_current, 0, kernel_start, 4096, false);
        if(vaddr == 0) {
            kerror("cannot find virtual address space to map video modes list to");
            vmm_pgunmap(vmm_current, VBE_DATA_VADDR);
            return -2;
        }
        vmm_pgmap(vmm_current, (uintptr_t) modes - offset, vaddr, VMM_FLAGS_PRESENT);
        modes = (uint16_t*) (vaddr + offset);
    } else modes = (uint16_t*) ((uintptr_t) modes - VBE_DATA_PADDR + VBE_DATA_VADDR); // video modes table comes with the controller info struct
    for(; modes[vbe_modes_len] != 0xFFFF; vbe_modes_len++); // count modes
    kdebug("found %u video mode(s) in video mode list at 0x%x", vbe_modes_len, modes);
    vbe_modes = kcalloc(vbe_modes_len, sizeof(vbe_mode_t));
    if(vbe_modes == NULL) {
        kerror("cannot allocate video modes list");
        if(map) vmm_pgunmap(vmm_current, (uintptr_t) modes);
        vmm_pgunmap(vmm_current, VBE_DATA_VADDR);
        return -3;
    }
    for(size_t i = 0; i < vbe_modes_len; i++) vbe_modes[i].mode = modes[i]; // copy mode numbers over
    if(map) vmm_pgunmap(vmm_current, (uintptr_t) modes); // unmap the video modes list

    /* get information on each mode */
    for(size_t i = 0; i < vbe_modes_len; i++) {
        vbe_modeinfo_t* mode = vbe_get_modeinfo(vbe_modes[i].mode);
        if(mode == NULL) {
            kerror("cannot get information on video mode 0x%x", vbe_modes[i].mode);
            goto shrink;
        }
        if(!(mode->attribs & (1 << 7))) {
            kwarn("skipping mode 0x%x since LFB is not supported", vbe_modes[i].mode);
            goto shrink;
        }
        if(mode->bpp <= 8) {
            kwarn("skipping mode 0x%x due to low BPP", vbe_modes[i].mode);
            goto shrink;
        }
        vbe_modes[i].width = mode->width; vbe_modes[i].height = mode->height; vbe_modes[i].bpp = mode->bpp; vbe_modes[i].pitch = mode->pitch; vbe_modes[i].framebuffer_ptr = mode->framebuffer;
        // kdebug("mode 0x%x: %ux%ux%u, framebuffer ptr 0x%x (%u bytes)", vbe_modes[i].mode, vbe_modes[i].width, vbe_modes[i].height, vbe_modes[i].bpp, vbe_modes[i].framebuffer_ptr, vbe_modes[i].pitch * vbe_modes[i].height);
        continue;
shrink: 
        if(i != vbe_modes_len - 1) memmove(&vbe_modes[i], &vbe_modes[i + 1], (vbe_modes_len - i - 1) * sizeof(vbe_mode_t));
        vbe_modes_len--; i--; vbe_modes = krealloc(vbe_modes, vbe_modes_len * sizeof(vbe_mode_t));
    }
    kdebug("found %u usable video mode(s)", vbe_modes_len);

    /* check for video mode override in cmdline */
    char* mode_override = cmdline_find_kvp("vbe_mode");
    if(mode_override != NULL) {
        uint16_t mode = strtoul(mode_override, NULL, 16);
        for(size_t i = 0; i < vbe_modes_len; i++) {
            if(vbe_modes[i].mode == mode) {
                vbe_mode_current = &vbe_modes[i];
                break;
            }
        }
        if(vbe_mode_current == NULL) kerror("VBE mode 0x%x is not available", mode);
    }

    /* check for video resolution specification in cmdline */
    if(vbe_mode_current == NULL) {
        mode_override = cmdline_find_kvp("resolution");
        if(mode_override != NULL) {
            size_t width = strtoul(mode_override, &mode_override, 10);
            size_t height = strtoul(&mode_override[1], &mode_override, 10);
            size_t bpp = (*mode_override == '\0') ? 0 : strtoul(&mode_override[1], NULL, 10); // bpp is optional; if not specified, the driver will find the highest possible BPP mode
            vbe_mode_current = vbe_find_mode(width, height, bpp);
        }
    }

    /* use built-in resolution */
    if(vbe_mode_current == NULL) {
        vbe_mode_current = vbe_find_mode(VBE_FALLBACK_WIDTH, VBE_FALLBACK_HEIGHT, 0);
        if(vbe_mode_current == NULL) {
            kerror("cannot find any suitable resolution");
            kfree(vbe_modes);
            vmm_pgunmap(vmm_current, VBE_DATA_VADDR);
            return -4;
        }   
    }

    /* find virtual address space for framebuffer */
    size_t fb_size = vbe_mode_current->pitch * vbe_mode_current->height;
    vbe_framebuffer = (void*) vmm_first_free(vmm_current, kernel_end, 0xFFFFFFFF, fb_size, false);
    if(vbe_framebuffer == NULL) {
        kerror("cannot find virtual address space for framebuffer");
        kfree(vbe_modes);
        vmm_pgunmap(vmm_current, VBE_DATA_VADDR);
        return -5;
    } else kinfo("framebuffer: 0x%x, size: %u bytes", vbe_framebuffer, fb_size);

    /* set video mode */
    kinfo("setting video mode to 0x%x (%ux%ux%u)", vbe_mode_current->mode, vbe_mode_current->width, vbe_mode_current->height, vbe_mode_current->bpp);
    // kdebug("setting video mode to 0x%x (%ux%ux%u)", vbe_mode_current->mode, vbe_mode_current->width, vbe_mode_current->height, vbe_mode_current->bpp);
    if(!vbe_set_mode(vbe_mode_current->mode)) {
        kerror("setting video mode failed");
        kfree(vbe_modes);
        vmm_pgunmap(vmm_current, VBE_DATA_VADDR);
        return -6;
    }

    /* map framebuffer to virtual address space */
    vmm_map(vmm_current, vbe_mode_current->framebuffer_ptr, (uintptr_t) vbe_framebuffer, fb_size, VMM_FLAGS_PRESENT | VMM_FLAGS_RW | VMM_FLAGS_CACHE | VMM_FLAGS_GLOBAL); // writeback cache

    vbe_fbuf_impl.width = vbe_mode_current->width; vbe_fbuf_impl.height = vbe_mode_current->height; vbe_fbuf_impl.pitch = vbe_mode_current->pitch;
    switch(vbe_mode_current->bpp) {
        case 15: vbe_fbuf_impl.type = FBUF_15BPP_RGB555; break;
        case 16: vbe_fbuf_impl.type = FBUF_16BPP_RGB565; break;
        case 24: vbe_fbuf_impl.type = FBUF_24BPP_BGR888; break;
        case 32: vbe_fbuf_impl.type = FBUF_32BPP_RGB888; break;
    }
    vbe_fbuf_impl.framebuffer = vbe_framebuffer;
    vbe_fbuf_impl.backbuffer = NULL; vbe_fbuf_impl.flip = NULL; // TODO: double buffering
    
    fbuf_impl = &vbe_fbuf_impl;
    term_impl = &fbterm_hook;

    memset(vbe_fbuf_impl.framebuffer, 0, vbe_fbuf_impl.pitch * vbe_fbuf_impl.height); // clear framebuffer

    return 0;
}