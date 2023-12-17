#ifndef IDE_DEVFS_H
#define IDE_DEVFS_H

#include <kmod.h>
#include <fs/devfs.h>

uint64_t ide_devfs_read(vfs_node_t* node, uint64_t offset, uint64_t size, uint8_t* buf);
uint64_t ide_devfs_write(vfs_node_t* node, uint64_t offset, uint64_t size, const uint8_t* buf);
bool ide_devfs_open(vfs_node_t* node, bool read, bool write);
void ide_devfs_close(vfs_node_t* node);

#endif
