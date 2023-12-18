#ifndef IDE_REGS_H
#define IDE_REGS_H

#include <stddef.h>
#include <stdint.h>
#include <arch/x86cpu/asm.h>

#include "devtree_defs.h"

/* task file */
#define IDE_REG_DATA                    0x00 // <-- BAR0 + 0
#define IDE_REG_ERROR                   0x01 // read
#define IDE_REG_FEATURES                0x01 // write
#define IDE_REG_SECCNT0                 0x02
#define IDE_REG_LBA0                    0x03
#define IDE_REG_LBA1                    0x04
#define IDE_REG_LBA2                    0x05
#define IDE_REG_HDDEVSEL                0x06 // also contains bits 24-27 of LBA in LBA28 mode
#define IDE_REG_CMD                     0x07 // write
#define IDE_REG_STAT                    0x07 // read
#define IDE_REG_SECCNT1                 0x08 // overlaps with IDE_REG_SECCNT0
#define IDE_REG_LBA3                    0x09 // overlaps with IDE_REG_LBA0
#define IDE_REG_LBA4                    0x0A // overlaps with IDE_REG_LBA1
#define IDE_REG_LBA5                    0x0B // overlaps with IDE_REG_LBA2
#define IDE_REG_CTRL                    0x0C // write <-- BAR1 + 2
#define IDE_REG_ALTSTAT                 0x0C // read
#define IDE_REG_DEVADDR                 0x0D

/* status register bitmasks */
#define IDE_SR_ERR                      (1 << 0)
#define IDE_SR_IDX                      (1 << 1)
#define IDE_SR_CORR                     (1 << 2)
#define IDE_SR_DRQ                      (1 << 3)
#define IDE_SR_DSC                      (1 << 4)
#define IDE_SR_DF                       (1 << 5)
#define IDE_SR_DRDY                     (1 << 6)
#define IDE_SR_BSY                      (1 << 7)

/* control register bitmasks */
#define IDE_CR_NIEN                     (1 << 1) // interrupt disable
#define IDE_CR_SRST                     (1 << 2) // software reset (set, wait 5uS, then clear)
#define IDE_CR_HOB                      (1 << 7) // for accessing SECCNT1, LBA3, LBA4 and LBA5

/* HDDEVSEL bitmasks */
#define IDE_HDSR_BASE                   ((1 << 5) | (1 << 7)) // required bits
#define IDE_HDSR_LBA                    (1 << 6) // LBA mode
#define IDE_HDSR_DRV                    (1 << 4) // drive select

/* ATA commands */
#define ATA_CMD_READ_PIO                0x20
#define ATA_CMD_READ_PIO_EXT            0x24
#define ATA_CMD_READ_DMA                0xC8
#define ATA_CMD_READ_DMA_EXT            0x25
#define ATA_CMD_WRITE_PIO               0x30
#define ATA_CMD_WRITE_PIO_EXT           0x34
#define ATA_CMD_WRITE_DMA               0xCA
#define ATA_CMD_WRITE_DMA_EXT           0x35
#define ATA_CMD_FLUSH_CACHE             0xE7
#define ATA_CMD_FLUSH_CACHE_EXT         0xEA
#define ATA_CMD_PACKET                  0xA0
#define ATA_CMD_ID_PACKET               0xA1
#define ATA_CMD_ID                      0xEC

/* ATA identify buffer offset */
#define ATA_ID_DEVTYPE                  0
#define ATA_ID_CYLS                     2 // number of cylinders
#define ATA_ID_HEADS                    6 // number of heads
#define ATA_ID_SECTS                    12 // number of sectors per cylinder
#define ATA_ID_SERIAL                   20
#define ATA_ID_MODEL                    54
#define ATA_ID_CAPABILITIES             98
#define ATA_ID_FIELDVALID               106
#define ATA_ID_MAX_LBA                  120
#define ATA_ID_CMDSETS                  164
#define ATA_ID_MAX_LBA_EXT              200

static inline void ide_write_byte(ide_channel_devtree_t* channel, uint16_t reg, uint8_t val) {
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5) // access overlapped regs
        ide_write_byte(channel, IDE_REG_CTRL, IDE_CR_HOB | ((channel->irq_disable) ? IDE_CR_NIEN : 0));
    if(reg < IDE_REG_SECCNT1)
        outb(channel->io_base + reg, val);
    else if(reg < IDE_REG_CTRL)
        outb(channel->io_base - (IDE_REG_SECCNT1 - IDE_REG_SECCNT0) + reg, val);
    else if(reg <= IDE_REG_DEVADDR)
        outb(channel->ctrl_base - (IDE_REG_CTRL - 2) + reg, val);
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5)
        ide_write_byte(channel, IDE_REG_CTRL, ((channel->irq_disable) ? IDE_CR_NIEN : 0));
}

static inline void ide_write_word(ide_channel_devtree_t* channel, uint16_t reg, uint16_t val) {
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5) // access overlapped regs
        ide_write_byte(channel, IDE_REG_CTRL, IDE_CR_HOB | ((channel->irq_disable) ? IDE_CR_NIEN : 0));
    if(reg < IDE_REG_SECCNT1)
        outw(channel->io_base + reg, val);
    else if(reg < IDE_REG_CTRL)
        outw(channel->io_base - (IDE_REG_SECCNT1 - IDE_REG_SECCNT0) + reg, val);
    else if(reg <= IDE_REG_DEVADDR)
        outw(channel->ctrl_base - (IDE_REG_CTRL - 2) + reg, val);
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5)
        ide_write_byte(channel, IDE_REG_CTRL, ((channel->irq_disable) ? IDE_CR_NIEN : 0));
}

static inline uint8_t ide_read_byte(ide_channel_devtree_t* channel, uint16_t reg) {
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5) // access overlapped regs
        ide_write_byte(channel, IDE_REG_CTRL, IDE_CR_HOB | ((channel->irq_disable) ? IDE_CR_NIEN : 0));
    uint8_t ret = 0;
    if(reg < IDE_REG_SECCNT1)
        ret = inb(channel->io_base + reg);
    else if(reg < IDE_REG_CTRL)
        ret = inb(channel->io_base - (IDE_REG_SECCNT1 - IDE_REG_SECCNT0) + reg);
    else if(reg <= IDE_REG_DEVADDR)
        ret = inb(channel->ctrl_base - (IDE_REG_CTRL - 2) + reg);
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5)
        ide_write_byte(channel, IDE_REG_CTRL, ((channel->irq_disable) ? IDE_CR_NIEN : 0));
    return ret;
}

static inline uint16_t ide_read_word(ide_channel_devtree_t* channel, uint16_t reg) {
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5) // access overlapped regs
        ide_write_byte(channel, IDE_REG_CTRL, IDE_CR_HOB | ((channel->irq_disable) ? IDE_CR_NIEN : 0));
    uint16_t ret = 0;
    if(reg < IDE_REG_SECCNT1)
        ret = inw(channel->io_base + reg);
    else if(reg < IDE_REG_CTRL)
        ret = inw(channel->io_base - (IDE_REG_SECCNT1 - IDE_REG_SECCNT0) + reg);
    else if(reg <= IDE_REG_DEVADDR)
        ret = inw(channel->ctrl_base - (IDE_REG_CTRL - 2) + reg);
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5)
        ide_write_byte(channel, IDE_REG_CTRL, ((channel->irq_disable) ? IDE_CR_NIEN : 0));
    return ret;
}

static inline void ide_write_word_n(ide_channel_devtree_t* channel, uint16_t reg, const uint16_t* buf, size_t word_len) {
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5) // access overlapped regs
        ide_write_byte(channel, IDE_REG_CTRL, IDE_CR_HOB | ((channel->irq_disable) ? IDE_CR_NIEN : 0));
    if(reg < IDE_REG_SECCNT1)
        reg = channel->io_base + reg;
    else if(reg < IDE_REG_CTRL)
        reg = channel->io_base - (IDE_REG_SECCNT1 - IDE_REG_SECCNT0) + reg;
    else if(reg <= IDE_REG_DEVADDR)
        reg = channel->ctrl_base - (IDE_REG_CTRL - 2) + reg;
    for(size_t i = 0; i < word_len; i++) outw(reg, buf[i]); // TODO: using outsw might be better
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5)
        ide_write_byte(channel, IDE_REG_CTRL, ((channel->irq_disable) ? IDE_CR_NIEN : 0));
}

static inline uint16_t* ide_read_word_n(ide_channel_devtree_t* channel, uint16_t reg, uint16_t* buf, size_t word_len) {
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5) // access overlapped regs
        ide_write_byte(channel, IDE_REG_CTRL, IDE_CR_HOB | ((channel->irq_disable) ? IDE_CR_NIEN : 0));
    if(reg < IDE_REG_SECCNT1)
        reg = channel->io_base + reg;
    else if(reg < IDE_REG_CTRL)
        reg = channel->io_base - (IDE_REG_SECCNT1 - IDE_REG_SECCNT0) + reg;
    else if(reg <= IDE_REG_DEVADDR)
        reg = channel->ctrl_base - (IDE_REG_CTRL - 2) + reg;
    if(buf != NULL)
        for(size_t i = 0; i < word_len; i++) buf[i] = inw(reg); // TODO: using insw might be better
    else
        for(size_t i = 0; i < word_len; i++) inw(reg); // read to nowhere (i.e. discard)
    if(reg >= IDE_REG_SECCNT1 && reg <= IDE_REG_LBA5)
        ide_write_byte(channel, IDE_REG_CTRL, ((channel->irq_disable) ? IDE_CR_NIEN : 0));
    return buf;
}

static inline void ide_set_nien(ide_channel_devtree_t* channel, uint8_t nien) {
    channel->irq_disable = nien;
    ide_write_byte(channel, IDE_REG_CTRL, ((nien) ? IDE_CR_NIEN : 0));
}

#endif
