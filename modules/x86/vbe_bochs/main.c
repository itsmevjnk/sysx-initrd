#include <kmod.h>
#include <mm/vmm.h>
#include <mm/addr.h>
#include <stdlib.h>
#include <string.h>
#include <drivers/pci.h>
#include <hal/fbuf.h>
#include "io.h"

/* default (fallback) resolution */
#define VBE_FALLBACK_WIDTH                      640
#define VBE_FALLBACK_HEIGHT                     480

static pci_devtree_t* vbe_devtree_node = NULL;

static fbuf_t vbe_fbuf_impl;
static uintptr_t vbe_fbuf_base;
static size_t vbe_fbuf_size;

static bool vbe_unload_handler(fbuf_t* impl) {
    vmm_unmap(vmm_kernel, (uintptr_t) impl->framebuffer, impl->pitch * impl->height); // unmap framebuffer from memory
    return true;
}

static void vbe_scroll_up(fbuf_t* impl, size_t lines) {
    uintptr_t new_ptr = (uintptr_t) impl->framebuffer + lines * impl->pitch; // new framebuffer ptr
    if(new_ptr + impl->pitch * impl->height > vbe_fbuf_base + vbe_fbuf_size) {
        /* framebuffer overrun - reset to top */
        memmove((void*) vbe_fbuf_base, (void*) new_ptr, impl->pitch * (impl->height - lines));
        vbe_write_reg(VBE_DISPI_INDEX_Y_OFFSET, 0);
        impl->framebuffer = (void*) vbe_fbuf_base;
    } else {
        /* set new Y offset */
        vbe_write_reg(VBE_DISPI_INDEX_Y_OFFSET, (new_ptr - vbe_fbuf_base) / impl->pitch);
        impl->framebuffer = (void*) new_ptr;
    }
}

static void vbe_scroll_down(fbuf_t* impl, size_t lines) {
    uintptr_t new_ptr = (uintptr_t) impl->framebuffer - lines * impl->pitch; // new framebuffer ptr
    if(new_ptr < vbe_fbuf_base) {
        /* we can't go any further */
        new_ptr = vbe_fbuf_base;
        memmove((void*) new_ptr, impl->framebuffer, (impl->height - lines) * impl->pitch);
    }
    vbe_write_reg(VBE_DISPI_INDEX_Y_OFFSET, (new_ptr - vbe_fbuf_base) / impl->pitch);
    impl->framebuffer = (void*) new_ptr;
}

int32_t kmod_init(elf_prgload_t* load_result, size_t load_result_len) {
    /* check device availability */
    vbe_devtree_node = (pci_devtree_t*) devtree_traverse_path(NULL, "/pci/by-id/1234:1111");
    if(vbe_devtree_node == NULL) {
        kdebug("BGA device not present");
        return -1;
    } else kdebug("found BGA device on %02x:%02x.%x", vbe_devtree_node->bus, vbe_devtree_node->dev, vbe_devtree_node->func);
    if(mutex_test(&vbe_devtree_node->header.in_use)) {
        kerror("BGA device is in use");
        return -3;
    }

    /* check BGA version */
    uint16_t vbe_id = vbe_read_reg(VBE_DISPI_INDEX_ID);
    if(vbe_id < VBE_DISPI_ID2) {
        kdebug("unsupported BGA version 0x%04x", vbe_id);
        return -2;
    } else kdebug("BGA version 0x%04x", vbe_id);

    /* get framebuffer base and size */
    vbe_fbuf_base = pci_cfg_read_dword(vbe_devtree_node->bus, vbe_devtree_node->dev, vbe_devtree_node->func, PCI_CFG_H1_BAR0);
    if(vbe_fbuf_base & 1) {
        kerror("BAR0 of BGA device is not a memory address - invalid device?");
        return -1;
    }
    vbe_fbuf_base &= 0xFFFFFFF0;
    vbe_fbuf_size = vbe_read_reg(VBE_DISPI_INDEX_VIDEO_MEMORY_64K) * 65536;
    kdebug("framebuffer base: 0x%x, size: %uK", vbe_fbuf_base, vbe_fbuf_size / 1024);

    vbe_fbuf_impl.elf_segments = load_result; vbe_fbuf_impl.num_elf_segments = load_result_len;
    vbe_fbuf_impl.unload = &vbe_unload_handler;

    kinfo("Bochs Graphics Adapter driver for SysX");

    /* check if cmdline specifies that the kernel will use this driver */
    const char* fbdrv_override = cmdline_find_kvp("fbdrv");
    if(fbdrv_override != NULL) {
        if(!strcmp(fbdrv_override, "vbe_bochs")) {
            kinfo("force-loading module as specified by kernel cmdline");
        } else {
            kerror("kernel cmdline specifies another framebuffer driver module - exiting");
            return -11;
        }
    } else if(fbuf_impl != NULL) {
        kinfo("unloading existing framebuffer implementation");
        fbuf_unload();
    }

    /* read desired resolution */
    const char* res_override = cmdline_find_kvp("resolution");
    uint16_t mode_w = VBE_FALLBACK_WIDTH, mode_h = VBE_FALLBACK_HEIGHT, mode_bpp = 32;
    if(res_override != NULL) {
        mode_w = strtoul(res_override, (char**) &res_override, 10);
        mode_h = strtoul(res_override, (char**) &res_override, 10);
        if(*res_override != '\0') mode_bpp = strtoul(res_override, NULL, 10);
    }
    if(mode_bpp != 15 && mode_bpp != 16 && mode_bpp != 24 && mode_bpp != 32) {
        kwarn("invalid BPP setting %u", mode_bpp);
        mode_bpp = 32;
    }
    if(mode_w % 8 != 0 || mode_h % 8 != 0) {
        kwarn("unaligned resolution %ux%u - rounding down to nearest multiple of 8", mode_w, mode_h);
        mode_w -= mode_w % 8;
        mode_h -= mode_h % 8;
    }

    /* fill in mode settings */
    vbe_fbuf_impl.width = mode_w; vbe_fbuf_impl.height = mode_h;
    switch(mode_bpp) {
        case 15: vbe_fbuf_impl.type = FBUF_15BPP_RGB555; vbe_fbuf_impl.pitch = mode_w * 2; break;
        case 16: vbe_fbuf_impl.type = FBUF_16BPP_RGB565; vbe_fbuf_impl.pitch = mode_w * 2; break;
        case 24: vbe_fbuf_impl.type = FBUF_24BPP_BGR888; vbe_fbuf_impl.pitch = mode_w * 3; break;
        case 32: vbe_fbuf_impl.type = FBUF_32BPP_RGB888; vbe_fbuf_impl.pitch = mode_w * 4; break;
    }
    vbe_fbuf_impl.backbuffer = NULL; vbe_fbuf_impl.flip = NULL; // TODO: double buffering
    vbe_fbuf_impl.scroll_up = &vbe_scroll_up; vbe_fbuf_impl.scroll_down = &vbe_scroll_down;

    /* map framebuffer */
    vbe_fbuf_base = vmm_alloc_map(vmm_kernel, vbe_fbuf_base, vbe_fbuf_size, kernel_end, UINTPTR_MAX, false, VMM_FLAGS_PRESENT | VMM_FLAGS_GLOBAL | VMM_FLAGS_CACHE | VMM_FLAGS_RW);
    if(!vbe_fbuf_base) {
        kerror("cannot map framebuffer");
        return -4;
    }
    vbe_fbuf_impl.framebuffer = (void*) vbe_fbuf_base;

    mutex_acquire(&vbe_devtree_node->header.in_use); // lock device for exclusive use

    /* set video mode */
    kinfo("setting video mode to %ux%ux%u", mode_w, mode_h, mode_bpp);
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write_reg(VBE_DISPI_INDEX_XRES, mode_w);
    vbe_write_reg(VBE_DISPI_INDEX_YRES, mode_h);
    vbe_write_reg(VBE_DISPI_INDEX_BPP, mode_bpp);
    vbe_write_reg(VBE_DISPI_INDEX_VIRT_WIDTH, mode_w);
    vbe_write_reg(VBE_DISPI_INDEX_X_OFFSET, 0);
    vbe_write_reg(VBE_DISPI_INDEX_Y_OFFSET, 0); // TODO: accelerated scrolling
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
    
    fbuf_impl = &vbe_fbuf_impl;
    term_impl = &fbterm_hook;

    kinfo("Bochs Graphics Adapter initialized successfully");

    return 0;
}