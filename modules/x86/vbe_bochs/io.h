#ifndef VBE_BOCHS_IO_H
#define VBE_BOCHS_IO_H

#include <kmod.h>
#include <arch/x86cpu/asm.h>

/* IO ports */
#define VBE_DISPI_IOPORT_INDEX              0x01CE
#define VBE_DISPI_IOPORT_DATA               0x01CF

/* indexes */
#define VBE_DISPI_INDEX_ID                  0
#define VBE_DISPI_INDEX_XRES                1
#define VBE_DISPI_INDEX_YRES                2
#define VBE_DISPI_INDEX_BPP                 3
#define VBE_DISPI_INDEX_ENABLE              4
#define VBE_DISPI_INDEX_BANK                5
#define VBE_DISPI_INDEX_VIRT_WIDTH          6
#define VBE_DISPI_INDEX_VIRT_HEIGHT         7
#define VBE_DISPI_INDEX_X_OFFSET            8
#define VBE_DISPI_INDEX_Y_OFFSET            9

/* enable/disable VBE */
#define VBE_DISPI_DISABLED                  0x00
#define VBE_DISPI_ENABLED                   0x01
#define VBE_DISPI_LFB_ENABLED               0x40 // enable LFB
#define VBE_DISPI_NOCLEARMEM                0x80 // disable clearing memory on mode switch

/* BGA version IDs */
#define VBE_DISPI_ID0                       0xB0C0
#define VBE_DISPI_ID1                       0xB0C1
#define VBE_DISPI_ID2                       0xB0C2 // minimum supported version (for >=15BPP modes)
#define VBE_DISPI_ID3                       0xB0C3
#define VBE_DISPI_ID4                       0xB0C4
#define VBE_DISPI_ID5                       0xB0C5

/* BPP settings */
#define VBE_DISPI_BPP_4                     0x04
#define VBE_DISPI_BPP_8                     0x08
#define VBE_DISPI_BPP_15                    0x0F
#define VBE_DISPI_BPP_16                    0x10
#define VBE_DISPI_BPP_24                    0x18
#define VBE_DISPI_BPP_32                    0x20

static inline void vbe_write_reg(uint16_t index, uint16_t data) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, data);
}

static inline uint16_t vbe_read_reg(uint16_t index) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}

#endif
