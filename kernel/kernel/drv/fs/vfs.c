// Virtual File System Driver (vfs.c)
// Copyright (C) 2025 Skye310 (Galaxy Computing)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <kernel/devcfg.h>
#ifdef VFS

#include <kernel/vfs.h>
#include <kernel/liballoc.h>
#include <kernel/devreg.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define DEFAULT_BLOCKDEVICES_SIZE 512

struct vfs_block_device *blockdevices;
uint32_t last_blockdevice;
uint32_t blockdevices_size;

void vfs_init(void) {
    blockdevices = (struct vfs_block_device*)kmalloc(sizeof(struct vfs_block_device) * DEFAULT_BLOCKDEVICES_SIZE);
    blockdevices_size = DEFAULT_BLOCKDEVICES_SIZE;
    last_blockdevice = 0;
}

struct vfs_block_device *vfs_find_block_device_by_path(const char* path) {
    char buf[17];
    for (size_t i = 0; i < strlen(path); i++) {
        if (path[i] == '@') { // paths look like volume@folder/file
            buf[i] = 0;
            break;
        }
        if (i > 16) {
            return NULL; // this is an invalid path
        }
        buf[i] = path[i];
    }
    for (uint32_t i = 0; i < last_blockdevice; i++) {
        if (blockdevices[i].mounted) {
            if (!strcmp(blockdevices[i].mountpoint->name, buf)) {
                return &blockdevices[i];
            }
        }
    }
    return NULL; // the referenced volume doesn't exist
}

struct vfs_block_device *vfs_get_block_device(uint32_t id) {
    if (last_blockdevice - 1 > id) {
        return NULL; // this mountpoint doesn't exist
    }
    return &blockdevices[id];
}

uint32_t vfs_register_blockdevice(struct vfs_block_device *newblockdevice) {
    // need to do this weird thing or else two of these calls running at the same time would cause scary implosion of both block devices
    __sync_fetch_and_add(&last_blockdevice, 1);

    if (last_blockdevice-1 >= blockdevices_size) {
        blockdevices = (struct vfs_block_device*)krealloc((void*)blockdevices, sizeof(struct vfs_block_device) * (blockdevices_size+DEFAULT_BLOCKDEVICES_SIZE));
        blockdevices_size = blockdevices_size+DEFAULT_BLOCKDEVICES_SIZE;
    }

    memcpy(&blockdevices[last_blockdevice-1], newblockdevice, sizeof(struct vfs_block_device));
    blockdevices[last_blockdevice-1].id = last_blockdevice;
    return last_blockdevice;
}

struct vfs_mount_point *vfs_mount_direct(struct vfs_block_device *blockdevice, const struct vfs_mount_point *mp) {
    blockdevice->mountpoint = (struct vfs_mount_point*)kmalloc(sizeof(struct vfs_mount_point));
    memcpy(blockdevice->mountpoint, mp, sizeof(struct vfs_mount_point));

    return blockdevice->mountpoint;
}

/*struct vfs_mount_point *vfs_mount(struct vfs_block_device *blockdevice) {
    
    return NULL;
}*/

/*struct vfs_mount_point *vfs_mount_by_id(uint32_t blockdevice) {
    return vfs_mount(&blockdevices[blockdevice]);
}*/

/*FILE *vfs_get_file(const char *filename, const char *mode) {
    //struct vfs_block_device *fdevice = vfs_find_block_device_by_path(filename)->mountpoint;
    // FILE isn't a real thing yet
    return NULL;
}*/

uint32_t vfs_read_blocks(unsigned char *dest, const struct vfs_block_device *blockdevice, const uint32_t blocks, const uint32_t index) {
    uint32_t bytesread = 0;
    for (unsigned int i = 0; i < blocks; i++) {
        bytesread += blockdevice->devicedriver->block(dest+(i*blockdevice->blocksize), blockdevice, 0, i+index);
    }
    return bytesread;
} 

uint32_t vfs_write_blocks(unsigned char *data, const struct vfs_block_device *blockdevice, const uint32_t blocks, const uint32_t index) {
    uint32_t byteswritten = 0;
    for (unsigned int i = 0; i < blocks; i++) {
        byteswritten += blockdevice->devicedriver->block(data+(i*blockdevice->blocksize), blockdevice, 1, i+index);
    }
    return byteswritten;
} 

#endif
