#include "devfs.h"
#include <exec/task.h>
#include <string.h>

#include "devtree_defs.h"
#include "regs.h"

#define ATA_IO_MAX_SECTORS                          256 // maximum number of sectors to access in one go (so that all tasks have a fairer chance of accessing the channel/drive)

static int8_t ide_poll_channel(ide_channel_devtree_t* channel, bool check_status) {
    for(size_t i = 0; i < 4; i++) ide_read(channel, IDE_REG_ALTSTAT); // 400ns delay (TODO: improve this)
    
    /* wait for BSY flag to clear */
    uint8_t status;
    while((status = ide_read(channel, IDE_REG_STAT)) & IDE_SR_BSY) task_yield_noirq(); // TODO: is this the right thing to do?

    if(check_status) {
        /* check status bits */
        if(status & IDE_SR_ERR) {
            kdebug("%s/%s: ERR=1", channel->header.parent->name, channel->header.name);
            return -1;
        }
        if(status & IDE_SR_DF) {
            kdebug("%s/%s: DF=1", channel->header.parent->name, channel->header.name);
            return -2;
        }
        if(!(status & IDE_SR_DRQ)) {
            kdebug("%s/%s: DRQ=0", channel->header.parent->name, channel->header.name);
            return -3;
        }
    }

    return 0; // all good
}

static uint8_t ata_io_commands[4] = { // bit 0: direction, bit 1: LBA48
    ATA_CMD_READ_PIO,
    ATA_CMD_WRITE_PIO,
    ATA_CMD_READ_PIO_EXT,
    ATA_CMD_WRITE_PIO_EXT
};

static uint64_t ide_devfs_ata_stub(ide_dev_devtree_t* dev, bool write, uint64_t offset, uint64_t size, uint8_t* buf) {
    if(size == 0 || offset >= (dev->size << 9)) return 0; // nothing to be done here, period

    if(write & ((offset & 0x1FF) || (size & 0x1FF))) {
        uint8_t sect_buf[512];
        uint64_t ret = 0;
        while(size > 0) {
            uint64_t iter_size, iter_ret; // number of bytes to be written + number of bytes that have actually been written
            if((offset & 0x1FF) || (size < 512)) {
                /* read and re-write */
                iter_ret = ide_devfs_ata_stub(dev, false, offset & ~0x1FF, 512, sect_buf); // read the sector we will be writing back
                if(iter_ret != 512) {
                    kdebug("premature exit: iter_ret=%llu (!=512) trying to read LBA %llu to re-write -> returning %llu", iter_ret, offset >> 9, ret);
                    return ret;
                }

                if(offset & 0x1FF) {
                    /* unaligned offset */
                    iter_size = 512 - (offset & 0x1FF);
                    memcpy(&sect_buf[512 - iter_size], buf, iter_size);
                } else {
                    /* unaligned (<512) size */
                    iter_size = size;
                    memcpy(sect_buf, buf, iter_size);
                }

                iter_ret = ide_devfs_ata_stub(dev, true, offset & ~0x1FF, 512, sect_buf); // re-write the sector
                ret += (iter_ret == 512) ? iter_size : iter_ret;
                if(iter_ret != 512) {
                    kdebug("premature exit: iter_ret=%llu (!=512) trying to re-write LBA %llu -> returning %llu", iter_ret, offset >> 9, ret);
                    return ret;
                }
            } else {
                iter_size = size & 0x1FF; // try to write as any bytes as possible, while we remain within the 512-byte boundary
                iter_ret = ide_devfs_ata_stub(dev, true, offset, iter_size, buf);
                ret += iter_ret;
                if(iter_ret != iter_size) {
                    kdebug("premature exit: iter_ret=%llu (!=%llu) trying to write %llu sectors starting from LBA %llu -> returning %llu", iter_ret, iter_size, iter_size >> 9, offset >> 9, ret);
                    return ret;
                }
            }
            size -= iter_size; offset += iter_size; buf = &buf[iter_size]; // advance
        }
        return ret;
    }

    /* calculate LBA */
    uint64_t lba_start = offset >> 9; // starting LBA. >> 9 is short for /512
    uint64_t lba_end = (offset + size - 1) >> 9;
    if(lba_end >= dev->size) lba_end = dev->size - 1; // cut off if we're attempting to read past the disk's size
    uint64_t sec_cnt = lba_end - lba_start + 1; // number of sectors to be read/written
    if(sec_cnt > ATA_IO_MAX_SECTORS || (dev->addressing == ATA_ADDR_LBA48 && sec_cnt > UINT16_MAX) || (dev->addressing != ATA_ADDR_LBA48 && sec_cnt > UINT8_MAX)) {
        /* number of sectors to be accessed surpasses maximum for supported addressing mode - time to subdivide */
        uint64_t ret = 0;
        while(size > 0) {
            size_t iter_size = ((((dev->addressing == ATA_ADDR_LBA48) ? UINT16_MAX : UINT8_MAX) - 1) << 9) + (512 - (offset & 0x1FF)); // read the maximum number of bytes out of it, and try to align offset on the way
            if(iter_size > (ATA_IO_MAX_SECTORS << 9)) iter_size = ATA_IO_MAX_SECTORS << 9;
            if(iter_size > size) iter_size = size; // reading more than we can here
            uint64_t iter_ret = ide_devfs_ata_stub(dev, write, offset, iter_size, buf); // try again, one piece at a time
            ret += iter_ret;
            size -= iter_size; offset += iter_size; buf = &buf[iter_size];
            if(iter_ret != iter_size) {
                kdebug("premature exit: iter_ret=%llu (!=%llu) accessing (write=%u) offset=%llu iter_size=%llu -> returning %llu", iter_ret, iter_size, (write)?1:0, offset, iter_size, ret);
                break;
            }
        }
        return ret;
    }

    /* calculate parameters */
    uint8_t lba_io[6] = {0, 0, 0, 0, 0, 0}, head = 0;
    uint16_t cyl;
    switch(dev->addressing) {
        case ATA_ADDR_LBA48:
            lba_io[0]   = (lba_start & 0x0000000000FF) >> 0;
            lba_io[1]   = (lba_start & 0x00000000FF00) >> 8;
            lba_io[2]   = (lba_start & 0x000000FF0000) >> 16;
            lba_io[3]   = (lba_start & 0x0000FF000000) >> 24;
            lba_io[4]   = (lba_start & 0x00FF00000000) >> 32;
            lba_io[5]   = (lba_start & 0xFF0000000000) >> 40;
            break;
        case ATA_ADDR_LBA28:
            lba_io[0]   = (lba_start & 0x00000FF) >> 0;
            lba_io[1]   = (lba_start & 0x000FF00) >> 8;
            lba_io[2]   = (lba_start & 0x0FF0000) >> 16;
            head        = (lba_start & 0xF000000) >> 24;
            break;
        case ATA_ADDR_CHS:
            lba_io[0]   = (lba_start % dev->sects) + 1;
            cyl         = (lba_start + 1 - lba_io[0]) / (dev->cyls * dev->heads);
            lba_io[1]   = (cyl & 0x00FF) >> 0;
            lba_io[2]   = (cyl & 0xFF00) >> 8;
            head        = (lba_start / dev->sects) % dev->heads;
            break;
    }

    /* wait if drive is busy */
    ide_channel_devtree_t* channel = (ide_channel_devtree_t*) dev->header.parent;
    mutex_acquire(&channel->access); // wait until tasks have finished using this channel
    while(ide_read(channel, IDE_REG_STAT) & IDE_SR_BSY)
        task_yield_noirq(); // TODO: is this the right thing to do?

    /* select drive and addressing mode, and also send head number over */
    ide_write(channel, IDE_REG_HDDEVSEL, IDE_HDSR_BASE | head | ((dev->drive) ? IDE_HDSR_DRV : 0) | ((dev->addressing == ATA_ADDR_CHS) ? 0 : IDE_HDSR_LBA));

    /* write parameters */
    if(dev->addressing == ATA_ADDR_LBA48) {
        ide_write(channel, IDE_REG_SECCNT1, (uint8_t) (sec_cnt >> 8));
        ide_write(channel, IDE_REG_LBA3, lba_io[3]);
        ide_write(channel, IDE_REG_LBA4, lba_io[4]);
        ide_write(channel, IDE_REG_LBA5, lba_io[5]);
    }
    ide_write(channel, IDE_REG_SECCNT0, (uint8_t) (sec_cnt & 0xFF));
    ide_write(channel, IDE_REG_LBA0, lba_io[0]);
    ide_write(channel, IDE_REG_LBA1, lba_io[1]);
    ide_write(channel, IDE_REG_LBA2, lba_io[2]);

    /* send command and begin operation */
    ide_write(channel, IDE_REG_CMD, ata_io_commands[((write) ? (1 << 0) : 0) | ((dev->addressing == ATA_ADDR_LBA48) ? (1 << 1) : 0)]);
    int8_t poll_ret; // polling result
    uint64_t ret = 0;
    for(size_t i = 0; i < sec_cnt; i++) {
        poll_ret = ide_poll_channel(channel, true);
        if(poll_ret < 0) {
            if(write && ret > 0) ret -= 512; // last write failed
            kdebug("premature exit: ide_poll_channel returned %d -> returning %llu", poll_ret, ret);
            goto done;
        }

        /* drive is ready for reading/writing */
        if(write) {
            /* write sector - offset and size is guaranteed to be aligned, so there's nothing much to worry about here */
            ide_write_buf(channel, IDE_REG_DATA, (uint16_t*) buf, 256);
            buf = &buf[512]; // might be out of bound!
            ret += 512; // a whole sector written
        } else {
            /* read sector */
            uint16_t offset_off = offset & 0x1FF; // offset misalignment
            uint16_t out = 0; // number of words outputted from the drive
            uintptr_t buf_old = (uintptr_t) buf; // buffer pointer before reading
            if(offset_off) {
                /* discard first bytes */
                ide_read_buf(channel, IDE_REG_DATA, NULL, offset_off / 2); out += offset_off / 2; // discard whole words at a time
                if(offset_off & 1) {
                    *(buf++) = ide_read(channel, IDE_REG_DATA) >> 8; // discard low byte only for the last word
                    out++;
                }
            }

            uint16_t read = 512 - out * 2; if(read > size) read = size; // number of bytes to be read
            ide_read_buf(channel, IDE_REG_DATA, (uint16_t*) buf, read / 2); buf = &buf[read & ~1]; out += read / 2; // read words at a time
            if(read & 1) {
                *(buf++) = ide_read(channel, IDE_REG_DATA) & 0xFF; // keep low byte only for the last word
                out++;
            }
            
            if(out < 256) ide_read_buf(channel, IDE_REG_DATA, NULL, 256 - out); // there are still words left to be read from the buffer
            ret += (uintptr_t) buf - buf_old;
        }
    }

done:
    if(write) {
        /* flush cache */
        ide_write(channel, IDE_REG_CMD, (dev->addressing == ATA_ADDR_LBA48) ? ATA_CMD_FLUSH_CACHE_EXT : ATA_CMD_FLUSH_CACHE);
        ide_poll_channel(channel, false);
    }
    mutex_release(&channel->access);
    return ret;
}

uint64_t ide_devfs_read(vfs_node_t* node, uint64_t offset, uint64_t size, uint8_t* buf) {
    ide_dev_devtree_t* dev = node->link.ptr;
    kassert(dev != NULL);
    kassert(dev->header.size == sizeof(ide_dev_devtree_t));
    kassert(dev->devfs_node == node);

    if(!mutex_test(&dev->header.in_use)) return 0; // drive not opened yet

    if(dev->type) {
        /* ATAPI */
        kdebug("reading from ATAPI drives is not supported yet");
        return 0;
    } else {
        /* ATA */
        return ide_devfs_ata_stub(dev, false, offset, size, buf);
    }
}

uint64_t ide_devfs_write(vfs_node_t* node, uint64_t offset, uint64_t size, const uint8_t* buf) {
    ide_dev_devtree_t* dev = node->link.ptr;
    kassert(dev != NULL && dev->header.size == sizeof(ide_dev_devtree_t) && dev->devfs_node == node);

    if(!mutex_test(&dev->header.in_use)) return 0; // drive not opened yet

    if(dev->type) {
        /* ATAPI */
        kdebug("writing from ATAPI drives is not supported yet");
        return 0;
    } else {
        /* ATA */
        return ide_devfs_ata_stub(dev, true, offset, size, (uint8_t*) buf);
    }
}

bool ide_devfs_open(vfs_node_t* node, bool read, bool write) {
    (void) read; (void) write; // TODO: consider these params

    ide_dev_devtree_t* dev = node->link.ptr;
    kassert(dev != NULL && dev->header.size == sizeof(ide_dev_devtree_t) && dev->devfs_node == node);

    if(dev->type) {
        /* ATAPI */
        kdebug("ATAPI drives are not supported yet");
        return false;
    }

    if(!mutex_test(&dev->header.in_use)) mutex_acquire(&dev->header.in_use);
    else kdebug("device %s is already opened", node->name);

    return true;
}

void ide_devfs_close(vfs_node_t* node) {
    ide_dev_devtree_t* dev = node->link.ptr;
    kassert(dev != NULL && dev->header.size == sizeof(ide_dev_devtree_t) && dev->devfs_node == node);

    if(dev->type) {
        /* ATAPI */
        kdebug("ATAPI drives are not supported yet");
        return;
    }

    if(mutex_test(&dev->header.in_use)) mutex_release(&dev->header.in_use);
    else kdebug("device %s is already closed", node->name);
}
