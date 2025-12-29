// RAM Disk Driver (rdisk.c)
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
#ifdef RDISK

#include <kernel/devreg.h>
#include <kernel/rdisk.h>
#include <kernel/vfs.h>
#include <kernel/liballoc.h>
#include <string.h>

#define RDISK_BLOCK_SIZE 512

struct vfs_device_driver rdisk_vfsdrvinfo;

uint32_t rdisk_create(uint32_t blocks) {
    struct vfs_block_device tempdevice;
    tempdevice.devicedriver = &rdisk_vfsdrvinfo;
    tempdevice.blocksize = RDISK_BLOCK_SIZE;
    tempdevice.blocks = blocks;
    tempdevice.mounted = 0;
    tempdevice.ispartition = 2;

    // we're using the extraa field here as a pointer to the device contents in memory
    tempdevice.extraa = kmalloc(blocks*RDISK_BLOCK_SIZE);

    uint32_t id = vfs_register_blockdevice(&tempdevice);
    return id;
}

// standard vfs interface
uint32_t rdisk_block(unsigned char* data, const struct vfs_block_device* blockdevice, const uint8_t write, const uint32_t index) { 
    if (write) {
        // we are writing
        if (blockdevice->ispartition) {
            memcpy(((char*)(blockdevice->extraa))+(index*RDISK_BLOCK_SIZE), data, RDISK_BLOCK_SIZE);
        } else {
            // not supported yet
            return 0;
        }
    } else {
        // we are reading
        if (blockdevice->ispartition) {
            memcpy(data, ((char*)(blockdevice->extraa))+(index*RDISK_BLOCK_SIZE), RDISK_BLOCK_SIZE);
        } else {
            // not supported yet
            return 0;
        }
    }
    return RDISK_BLOCK_SIZE;
}

void rdisk_init(void) {
    rdisk_vfsdrvinfo.block = rdisk_block;
    rdisk_vfsdrvinfo.devicedriver = devreg_find_device_by_kid(RDISK);
}

#endif
