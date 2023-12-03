#ifndef VBE_GENERIC_BIOS_H
#define VBE_GENERIC_BIOS_H

#include <kmod.h>
#include <string.h>
#include <arch/x86cpu/int32.h>

#define VBE_DATA_PADDR                      0x8000 // where to tell BIOS to put data
#define VBE_DATA_VADDR                      (0xC0000000 + VBE_DATA_PADDR) // where we'll map VBE_DATA_PADDR to

/* VBE controller information structure */
typedef struct {
    char signature[4]; // must be VESA
    uint8_t ver_min; // minor version
    uint8_t ver_maj; // major version
    uint16_t oemstr_off; // offset of OEM string pointer
    uint16_t oemstr_seg; // segment of OEM string pointer
    uint32_t capabilities;
    uint16_t modes_off; // offset of video modes list
    uint16_t modes_seg; // segment of video modes list
    uint16_t memory; // video memory size in 64K blocks
    uint16_t vendor_off; // offset of vendor string
    uint16_t vendor_seg; // segment of vendor string
    uint16_t name_off; // offset of controller name string
    uint16_t name_seg; // segment of controller name string
    uint16_t rev_off; // offset of controller revision string
    uint16_t rev_seg; // segment of controller revision string
    uint8_t reserved[222];
    uint8_t oem_data[256];
} __attribute__((packed)) vbe_ctrlinfo_t;

/* VBE mode information structure */
typedef struct {
    uint16_t attribs; // deprecated - bit 7 indicates LFB support
    uint8_t win_a; // deprecated
    uint8_t win_b; // deprecated
    uint16_t granularity; // deprecated
    uint16_t win_sz;
    uint16_t seg_a;
    uint16_t seg_b;
    uint32_t win_funcptr; // deprecated
    uint16_t pitch; // bytes per line
    uint16_t width;
    uint16_t height;
    uint8_t w_char;
    uint8_t y_char;
    uint8_t planes;
    uint8_t bpp; // bits per pixel
    uint8_t banks;
    uint8_t mem_model;
    uint8_t bank_sz;
    uint8_t img_pages;
    uint8_t rsvd_1;
    uint8_t red_mask;
    uint8_t red_pos;
    uint8_t green_mask;
    uint8_t green_pos;
    uint8_t blue_mask;
    uint8_t blue_pos;
    uint8_t rsvd_mask;
    uint8_t rsvd_pos;
    uint8_t dircolor_attribs;
    uint32_t framebuffer; // physical address of framebuffer
    uint32_t offscreen_mem_off; // physical address of offscreen memory
    uint32_t offscreen_mem_sz; // size of offscreen memory
    uint8_t rsvd_2[206];
} __attribute__((packed)) vbe_modeinfo_t;

#define VBE_LINEAR_ADDR(seg, off)           ((seg << 4) + off)

static vbe_ctrlinfo_t* vbe_get_ctrlinfo() {
    vbe_ctrlinfo_t* result = (vbe_ctrlinfo_t*) VBE_DATA_VADDR;
    // memset(&result, 0, sizeof(vbe_ctrlinfo_t));
    memcpy(&result->signature, "VBE2", 4); // indicate VBE 2.0+ support
    int32_regs_t regs; memset(&regs, 0, sizeof(int32_regs_t));
    regs.ax = 0x4F00; regs.es = ((VBE_DATA_PADDR >> 16) << 12); regs.di = (VBE_DATA_PADDR & 0xFFFF);
    int32(0x10, &regs);
    if(!memcmp(&result->signature, "VESA", 4) && regs.ax == 0x004F) return (vbe_ctrlinfo_t*) VBE_DATA_VADDR;
    return NULL;
}

static vbe_modeinfo_t* vbe_get_modeinfo(uint16_t mode) {
    int32_regs_t regs; memset(&regs, 0, sizeof(int32_regs_t));
    regs.ax = 0x4F01; regs.cx = mode; regs.es = ((VBE_DATA_PADDR >> 16) << 12); regs.di = (VBE_DATA_PADDR & 0xFFFF);
    int32(0x10, &regs);
    if(regs.ax == 0x004F) return (vbe_modeinfo_t*) VBE_DATA_VADDR;
    return NULL;
}

static bool vbe_set_mode(uint16_t mode) {
    int32_regs_t regs; memset(&regs, 0, sizeof(int32_regs_t));
    regs.ax = 0x4F02; regs.bx = mode | 0x4000; // use LFB
    int32(0x10, &regs);
    return (regs.ax == 0x004F);
}

#endif
