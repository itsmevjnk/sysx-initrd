#include <kmod.h>
#include <drivers/pci.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <hal/timer.h>
#include <helpers/basecol.h>

#include "devtree_defs.h"
#include "regs.h"
#include "devfs.h"

/* fallback IO and control bases */
#define IDE_PRI_IO_BASE                 0x1F0
#define IDE_PRI_CTRL_BASE               0x3F6
#define IDE_SEC_IO_BASE                 0x170
#define IDE_SEC_CTRL_BASE               0x376

static bool ide_scan_devices(ide_channel_devtree_t* channel) {
    uint8_t buf[256 * 2]; // buffer for identify command

    /* prepare to create  */
    vfs_node_t* devfs_root = vfs_traverse_path(NULL, "/dev");
    if(devfs_root == NULL) {
        kerror("cannot find devfs root");
        return false;
    }

    for(size_t dr = 0; dr < 2; dr++) {
        /* select drive */
        ide_write_byte(channel, IDE_REG_HDDEVSEL, IDE_HDSR_BASE | ((dr) ? IDE_HDSR_DRV : 0));
        timer_delay_ms(1); // give it some time to change

        /* send ATA identify command */
        ide_write_byte(channel, IDE_REG_CMD, ATA_CMD_ID);
        timer_delay_ms(1);
        if(ide_read_byte(channel, IDE_REG_STAT) == 0) {
            kdebug("no drives on %s drive %u (SR = 0)", channel->header.name, dr);
            continue; // next drive
        }

        bool atapi = false;
        while(1) {
            uint8_t status = ide_read_byte(channel, IDE_REG_STAT);
            if(status & IDE_SR_ERR) {
                atapi = true; // not ATA (but could be ATAPI)
                break;
            }
            if(!(status & IDE_SR_BSY) && (status & IDE_SR_DRQ)) break; // ATA device (responding to ATA ID request)
        }

        if(atapi) {
            /* check for ATAPI */
            uint8_t cl = ide_read_byte(channel, IDE_REG_LBA1), ch = ide_read_byte(channel, IDE_REG_LBA2);
            if((cl == 0x14 && ch == 0xEB) || (cl == 0x69 && ch == 0x96)) {
                /* ATAPI device here */
                ide_write_byte(channel, IDE_REG_CMD, ATA_CMD_ID_PACKET);
                timer_delay_ms(1);
                while(1) { // TODO: is this needed?
                    uint8_t status = ide_read_byte(channel, IDE_REG_STAT);
                    if(status & IDE_SR_ERR) {
                        atapi = false; // not ATA and not ATAPI either - fail here
                        break;
                    }
                    if(!(status & IDE_SR_BSY) && (status & IDE_SR_DRQ)) break;
                }
                if(!atapi) {
                    kdebug("no drives on %s drive %u (ERR set in response to packet ID)", channel->header.name, dr);
                    continue;
                }
            } else {
                kdebug("no drives on %s drive %u (unknown LBA1=0x%x, LBA2=0x%x)", channel->header.name, dr, cl, ch);
                continue;
            }
        }

        /* lock channel, now that there's something here */
        if(!mutex_test(&channel->header.in_use)) mutex_acquire(&channel->header.in_use);

        /* read out the buffer */
        ide_read_word_n(channel, IDE_REG_DATA, (uint16_t*) buf, 256);

        /* store device information */
        ide_dev_devtree_t* dev = kcalloc(1, sizeof(ide_dev_devtree_t));
        dev->header.size = sizeof(ide_dev_devtree_t);
        if(dev == NULL) {
            kerror("cannot allocate memory for IDE device");
            return false;
        }
        dev->type = (atapi) ? 1 : 0;
        dev->drive = dr;
        dev->signature = *((uint16_t*) &buf[ATA_ID_DEVTYPE]);
        dev->capabilities = *((uint32_t*) &buf[ATA_ID_CAPABILITIES]); // NOTE: OSDev tutorials say this is 16-bit, but ACS-3 specs say it's 32-bit
        dev->cmdsets = *((uint64_t*) &buf[ATA_ID_CMDSETS]) & 0xFFFFFFFFFFFF; // NOTE: OSDev tutorials say this is 32-bit, but ACS-3 specs say it's 48-bit (for supported only)
        if(dev->capabilities & (1 << 9)) {
            /* LBA28/48 */
            if(dev->cmdsets & (1 << 26)) {
                kdebug("%s drive %u supports LBA48", channel->header.name, dr);
                dev->size = *((uint64_t*) &buf[ATA_ID_MAX_LBA_EXT]); // LBA48 command set supported
                dev->addressing = ATA_ADDR_LBA48;
            } else {
                kdebug("%s drive %u supports LBA28", channel->header.name, dr);
                dev->size = *((uint32_t*) &buf[ATA_ID_MAX_LBA]);
                dev->addressing = ATA_ADDR_LBA28;
            }
        } else {
            /* CHS */
            kdebug("%s drive %u only supports CHS addressing", channel->header.name, dr);
            dev->sects = *((uint16_t*) &buf[ATA_ID_SECTS]);
            dev->cyls = *((uint16_t*) &buf[ATA_ID_CYLS]);
            dev->heads = *((uint16_t*) &buf[ATA_ID_HEADS]);
            dev->size = dev->sects * dev->cyls * dev->heads;
            dev->addressing = ATA_ADDR_CHS;
        }
        for(size_t i = 0; i < 40; i += 2) {
            dev->model[i] = buf[ATA_ID_MODEL + i + 1];
            dev->model[i + 1] = buf[ATA_ID_MODEL + i];
        }

        /* generate node name */
        ksprintf(dev->header.name, "%u_%.13s", dr, dev->model);
        for(size_t i = 2; i < 15 && dev->header.name[i] != '\0'; i++) {
            if(!((dev->header.name[i] >= 'A' && dev->header.name[i] <= 'Z') || (dev->header.name[i] >= 'a' && dev->header.name[i] <= 'z') || (dev->header.name[i] >= '0' && dev->header.name[i] <= '9')))
                dev->header.name[i] = '_'; // remove invalid characters
        }

        /* create devfs node */
        memcpy(buf, (atapi) ? "sr" : "hd", 3); // ATAPI: /dev/srN, ATA: /dev/hdX
        bool name_found = false;
        for(size_t i = 0; i < UINTPTR_MAX; i++) { // TODO: have a more sensible limit
            if(atapi) ksprintf((char*) &buf[2], "%u", i);
            else basecol_encode(i, (char*) &buf[2], false);
            if(vfs_finddir(devfs_root, (char*) buf) == NULL) {
                name_found = true;
                break; // bingo!
            }
        }
        if(!name_found) {
            kerror("cannot find a suitable name for %s drive %u", channel->header.name, dr);
            kfree(dev);
            return false;
        }
        dev->devfs_node = devfs_create(devfs_root, &ide_devfs_read, &ide_devfs_write, &ide_devfs_open, &ide_devfs_close, NULL, true, dev->size * 512, (char*) buf);
        if(dev->devfs_node == NULL) {
            kerror("cannot create devfs node for %s drive %u", channel->header.name, dr);
            kfree(dev);
            return false;
        }
        dev->devfs_node->link.ptr = dev; // link back to device

        kdebug("    - %s (devfs name: %s): %s, type %u, sig 0x%04x, capabilities 0x%x, cmd sets 0x%llx, addr. mode %u, size: %llu sectors", dev->header.name, dev->devfs_node->name, dev->model, dev->type, dev->signature, dev->capabilities, dev->cmdsets, dev->addressing, dev->size);
        devtree_add_child((devtree_t*) channel, (devtree_t*) dev);

        /* test ATA device */
        if(!atapi) {
            memset(buf, 0, 512);
            vfs_open(dev->devfs_node, true, false);
            kdebug("sector 0 of %s (%llu bytes):", dev->devfs_node->name, vfs_read(dev->devfs_node, 0, 512, buf));
            for(size_t i = 0; i < 32; i++) kdebug("%03x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", i * 16, buf[i * 16 + 0], buf[i * 16 + 1], buf[i * 16 + 2], buf[i * 16 + 3], buf[i * 16 + 4], buf[i * 16 + 5], buf[i * 16 + 6], buf[i * 16 + 7], buf[i * 16 + 8], buf[i * 16 + 9], buf[i * 16 + 10], buf[i * 16 + 11], buf[i * 16 + 12], buf[i * 16 + 13], buf[i * 16 + 14], buf[i * 16 + 15]);
            vfs_close(dev->devfs_node);
        }
    }

    return true;
}

static bool ide_init(pci_devtree_t* dev, uint8_t prog_if) {
    kinfo("initializing IDE on device %s, prog IF 0x%02x", dev->header.name, prog_if);

    /* enumerate channels */
    ide_channel_devtree_t* channels = kcalloc(2, sizeof(ide_channel_devtree_t));
    if(channels == NULL) {
        kerror("cannot allocate memory for IDE channels");
        return false;
    }

    for(size_t ch = 0; ch < 2; ch++) {
        channels[ch].header.size = sizeof(ide_channel_devtree_t);
        channels[ch].header.type = DEVTREE_NODE_BUS;
        ksprintf(channels[ch].header.name, "ch%u", ch);

        /* read IO base */
        if(prog_if & (1 << 0)) {
            uint32_t bar = pci_cfg_read_dword(dev->bus, dev->dev, dev->func, (ch) ? PCI_CFG_H0_BAR2 : PCI_CFG_H0_BAR0);
            channels[ch].io_base = (bar) ? (bar & ~((1 << 0) | (1 << 1))) : ((ch) ? IDE_SEC_IO_BASE : IDE_PRI_IO_BASE);
        } else channels[ch].io_base = ((ch) ? IDE_SEC_IO_BASE : IDE_PRI_IO_BASE);
        
        /* read control base */
        if(prog_if & (1 << 2)) {
            uint32_t bar = pci_cfg_read_dword(dev->bus, dev->dev, dev->func, (ch) ? PCI_CFG_H0_BAR3 : PCI_CFG_H0_BAR1);
            channels[ch].ctrl_base = (bar) ? (bar & ~((1 << 0) | (1 << 1))) : ((ch) ? IDE_SEC_CTRL_BASE : IDE_PRI_CTRL_BASE);
        } else channels[ch].ctrl_base = ((ch) ? IDE_SEC_CTRL_BASE : IDE_PRI_CTRL_BASE);

        /* read bus master IDE base */
        if(prog_if & (1 << 7)) {
            uint32_t bar = pci_cfg_read_dword(dev->bus, dev->dev, dev->func, PCI_CFG_H0_BAR4);
            if(bar) channels[ch].bmide_base = (bar & ~((1 << 0) | (1 << 1))) + ((ch) ? 8 : 0);
        }

        kdebug(" - %s: IO base 0x%x, control port 0x%x, bus master IDE base 0x%x", channels[ch].header.name, channels[ch].io_base, channels[ch].ctrl_base, channels[ch].bmide_base);
        devtree_add_child((devtree_t*) dev, (devtree_t*) &channels[ch]);

        ide_set_nien(&channels[ch], 1); // disable interrupts for now

        /* enumerate devices on each channel */
        if(!ide_scan_devices(&channels[ch])) {
            kerror("fatal error occurred during device %s channel %u initialization", dev->header.name, ch);
            return false;
        }
    }

    return true;
}

int32_t kmod_init(elf_prgload_t* load_result, size_t load_result_len) {
    (void) load_result; (void) load_result_len;

    kinfo("PCI IDE controller driver for SysX");

    /* detect controllers */
    size_t detected = 0;
    devtree_t* pci_root = devtree_traverse_path(NULL, "/pci/by-geo");
    if(pci_root == NULL) kwarn("no PCI devices to detect");
    else {
        kinfo("detecting controllers on PCI bus");
        pci_devtree_t* pci_dev = (pci_devtree_t*) pci_root->first_child;
        while(pci_dev != NULL) {
            if(pci_dev->class == PCI_CLASS_STOR_CTRLR && pci_dev->subclass == PCI_MSC_IDE_CTRLR) {
                if(mutex_test(&pci_dev->header.in_use)) kerror("device %s is being held by another driver, skipping", pci_dev->header.name);
                else {
                    mutex_acquire(&pci_dev->header.in_use);
                    uint8_t prog_if = pci_read_progif(pci_dev->bus, pci_dev->dev, pci_dev->func);
                    uint8_t prog_if_orig = prog_if;
                    kdebug("found device %s, prog IF 0x%x", pci_dev->header.name, prog_if);
                    if((prog_if & (1 << 1)) && !(prog_if & (1 << 0))) {
                        kdebug(" - setting primary channel to PCI native mode");
                        prog_if |= (1 << 0);
                    }
                    if((prog_if & (1 << 3)) && !(prog_if & (1 << 2))) {
                        kdebug(" - setting secondary channel to PCI native mode");
                        prog_if |= (1 << 2);
                    }
                    if(prog_if != prog_if_orig) {
                        kdebug(" - writing new prog IF value");
                        pci_write_progif(pci_dev->bus, pci_dev->dev, pci_dev->func, prog_if);
                    }
                    if(!ide_init(pci_dev, prog_if)) {
                        kerror("fatal error occurred during device %s initialization", pci_dev->header.name);
                        return -1;
                    }
                    detected++;
                }
            }
            pci_dev = (pci_devtree_t*) pci_dev->header.next_sibling;
        }
    }
    if(!detected) {
        kerror("no devices detected, exiting");
        return -2;
    }
    kinfo("%u IDE controller(s) detected on PCI bus", detected);

    return 0;
}