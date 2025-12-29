// VFAT Driver (vfat.c)
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
#ifdef VFAT
#ifdef VFS

#include <kernel/vfat.h>
#include <kernel/vfs.h>
#include <kernel/devreg.h>
#include <kernel/liballoc.h>
#include <string.h>

struct vfs_fs_driver vfat_vfsdrvinfo;

int vfat_create_fatinfo(struct fat_info *fatinfo) {
    fatinfo->fat_size = (fatinfo->fatbs->table_size_16 == 0)? ((fat_extBS_32_t*)&(fatinfo->fatbs->extended_section))->table_size_32 : fatinfo->fatbs->table_size_16;
    fatinfo->total_sectors = (fatinfo->fatbs->total_sectors_16 == 0)? fatinfo->fatbs->total_sectors_32 : fatinfo->fatbs->total_sectors_16;
    fatinfo->root_dir_sectors = ((fatinfo->fatbs->root_entry_count * 32) + (fatinfo->fatbs->bytes_per_sector - 1)) / fatinfo->fatbs->bytes_per_sector;
    fatinfo->data_sectors = fatinfo->total_sectors - (fatinfo->fatbs->reserved_sector_count + (fatinfo->fatbs->table_count * fatinfo->fat_size) + fatinfo->root_dir_sectors);
    fatinfo->total_clusters = fatinfo->data_sectors / fatinfo->fatbs->sectors_per_cluster;
    
    if (fatinfo->fatbs->bytes_per_sector == 0) {
        // EXFAT
        fatinfo->fat_type = FAT_TYPE_EXFAT;
        return 2; // we don't support exfat
    }
    else if (fatinfo->total_clusters < 4085) {
        // FAT12
        fatinfo->fat_type = FAT_TYPE_12;
    } 
    else if (fatinfo->total_clusters < 65525) {
        // FAT16
        fatinfo->fat_type = FAT_TYPE_16;
    } 
    else {
        // FAT32
        fatinfo->fat_type = FAT_TYPE_32;
    }
    return 0;
}

int vfat_attempt_mount(struct vfs_block_device* blockdevice, const uint8_t rw) {
    unsigned char buf[blockdevice->blocksize];
    struct vfs_mount_point tempmount;
    struct fat_info* fatinfopointer;
    tempmount.rw = rw;
    tempmount.fsdriver = devreg_find_device_by_kid(VFAT);
    if (vfs_read_blocks(buf, blockdevice, 1, 0)) {
        tempmount.extra = kmalloc(sizeof(struct fat_info));
        fatinfopointer = (struct fat_info*)(tempmount.extra);
        fatinfopointer->fatbs = (fat_BS_t*)kmalloc(sizeof(fat_BS_t));
        fatinfopointer->blockdevice = blockdevice;
        memcpy(fatinfopointer->fatbs, &buf, sizeof(fat_BS_t));
        if (vfat_create_fatinfo(fatinfopointer)) {
            return 3;
        }
        vfs_mount_direct(blockdevice, &tempmount);
        return 0;
    } else {
        return 2;
    }
    return 1;
}

void vfat_init(void) {
    vfat_vfsdrvinfo.mount = vfat_attempt_mount;
    vfat_vfsdrvinfo.devicedriver = devreg_find_device_by_kid(VFAT);
}

#endif
#endif