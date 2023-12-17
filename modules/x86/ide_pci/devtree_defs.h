#ifndef IDE_DEVTREE_DEFS_H
#define IDE_DEVTREE_DEFS_H

#include <stddef.h>
#include <stdint.h>
#include <hal/devtree.h>
#include <fs/devfs.h>
#include <helpers/mutex.h>

/* IDE device node */
#define ATA_ADDR_CHS                    0
#define ATA_ADDR_LBA28                  1
#define ATA_ADDR_LBA48                  2
typedef struct {
    devtree_t header;
    // uint8_t channel; // 0 = primary, 1 = secondary
    uint8_t drive; // 0 = master, 1 = slave
    uint8_t type; // 0 = ATA, 1 = ATAPI
    uint16_t signature;
    uint32_t capabilities;
    uint64_t cmdsets;
    uint8_t addressing; // 0 = CHS, 1 = LBA28, 2 = LBA48 - applicable for ATA only
    uint16_t sects; // sectors per track - applicable for ATA in CHS only
    uint16_t cyls; // tracks per platter - applicable for ATA in CHS only
    uint16_t heads; // heads/platters - applicable for ATA in CHS only
    uint64_t size; // in sectors
    char model[41]; // drive model string
    vfs_node_t* devfs_node; // devfs node
} ide_dev_devtree_t;

/* IDE channel node */
typedef struct {
    devtree_t header;
    uint16_t io_base; // IO
    uint16_t ctrl_base; // control
    uint16_t bmide_base; // bus master IDE
    uint8_t irq_disable; // nIEN
    mutex_t access; // mutex for blocking channel access
} ide_channel_devtree_t;

#endif