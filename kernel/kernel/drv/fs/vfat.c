// VFAT Driver (vfat.c)
// Copyright (C) 2025-2026 Skye310 (Galaxy Computing)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <kernel/devcfg.h>
#ifdef VFAT
#ifdef VFS

#include <kernel/vfat.h>
#include <kernel/vfs.h>
#include <kernel/liballoc.h>
#include <string.h>
#include <ctype.h>

int vfat_attempt_mount(struct vfs_block_device* blockdevice, const bool rw);
int vfat_isvalid(struct vfs_block_device *blockdevice);
ssize_t vfat_accessfile(struct vfs_block_device* blockdevice, const bool write, struct vfs_file *descriptor, unsigned char* buffer, const size_t len, const size_t loc);

struct vfs_fs_driver vfat_vfsdrvinfo = {
    .mount      = &vfat_attempt_mount,
    .accessfile = &vfat_accessfile,
    .isvalid    = &vfat_isvalid,
    .name       = "VFAT"
};

inline uint32_t round_up_integer(uint32_t number, uint32_t multiple) {
    if (multiple == 0) return number;
    return ((number + multiple - 1) / multiple) * multiple;
}

inline uint32_t vfat_cluster_to_sector(uint32_t cluster, struct fat_info* fatinfo) {
    return ((cluster - 2) * fatinfo->fatbs->sectors_per_cluster) + fatinfo->first_data_sector;
}

uint32_t vfat_sectors_to_blocks(uint32_t fssector, struct vfs_mount_point* mount) {
    uint32_t block_size = mount->blockdevice->blocksize;
    uint32_t sector_size = ((struct fat_info*)mount->extra)->fatbs->bytes_per_sector;
    return round_up_integer(fssector * sector_size, block_size) / block_size;
}

uint32_t vfat_sector_to_lba(uint32_t fssector, struct vfs_mount_point* mount) {
    uint32_t block_size = mount->blockdevice->blocksize;
    uint32_t sector_size = ((struct fat_info*)mount->extra)->fatbs->bytes_per_sector;
    return fssector * sector_size / block_size;
}

// this assumes data is a pointer to a block of memory the size of at least 1 cluster
uint32_t vfat_read_cluster(unsigned char* data, uint32_t cluster, struct vfs_mount_point* mount) {
    struct fat_info* fatinfo = (struct fat_info*)(mount->extra);
    uint32_t sector_size = fatinfo->fatbs->bytes_per_sector;

    switch (fatinfo->fat_type) {
        case FAT_TYPE_12:
            unsigned int fat_offseta = cluster + (cluster / 2); // multiply by 1.5
            //unsigned int fat_sectora = fatinfo->first_fat_sector + (fat_offseta / sector_size);

            unsigned short table_valuea = *(unsigned short*)&fatinfo->fat_table[fat_offseta];
            table_valuea = (cluster & 1) ? table_valuea >> 4 : table_valuea & 0xfff;
            
            vfs_read_blocks(data, mount->blockdevice, vfat_sectors_to_blocks(fatinfo->fatbs->sectors_per_cluster, mount), vfat_sector_to_lba(vfat_cluster_to_sector(cluster, fatinfo), mount));

            return table_valuea;
        case FAT_TYPE_16:
            unsigned int fat_offsetb = cluster * 2;
            //unsigned int fat_sectorb = fatinfo->first_fat_sector + (fat_offsetb / sector_size);

            unsigned short table_valueb = *(unsigned short*)&fatinfo->fat_table[fat_offsetb];

            vfs_read_blocks(data, mount->blockdevice, vfat_sectors_to_blocks(fatinfo->fatbs->sectors_per_cluster, mount), vfat_sector_to_lba(vfat_cluster_to_sector(cluster, fatinfo), mount));

            return table_valueb;
        case FAT_TYPE_32:
            unsigned int fat_offsetc = cluster * 4;
            //unsigned int fat_sectorc = fatinfo->first_fat_sector + (fat_offsetc / sector_size);

            unsigned int table_valuec = *(unsigned int*)&fatinfo->fat_table[fat_offsetc];
            table_valuec &= 0x0FFFFFFF;

            vfs_read_blocks(data, mount->blockdevice, vfat_sectors_to_blocks(fatinfo->fatbs->sectors_per_cluster, mount), vfat_sector_to_lba(vfat_cluster_to_sector(cluster, fatinfo), mount));

            return table_valuec;
    }
    return 0;
}

// get next cluster in the chain without reading any data
uint32_t vfat_next_cluster(uint32_t cluster, struct vfs_mount_point* mount) {
    struct fat_info* fatinfo = (struct fat_info*)(mount->extra);
    uint32_t sector_size = fatinfo->fatbs->bytes_per_sector;

    switch (fatinfo->fat_type) {
        case FAT_TYPE_12:
            unsigned int fat_offseta = cluster + (cluster / 2); // multiply by 1.5
            //unsigned int fat_sectora = fatinfo->first_fat_sector + (fat_offseta / sector_size);

            unsigned short table_valuea = *(unsigned short*)&fatinfo->fat_table[fat_offseta];
            table_valuea = (cluster & 1) ? table_valuea >> 4 : table_valuea & 0xfff;

            return table_valuea;
        case FAT_TYPE_16:
            unsigned int fat_offsetb = cluster * 2;
            //unsigned int fat_sectorb = fatinfo->first_fat_sector + (fat_offsetb / sector_size);

            unsigned short table_valueb = *(unsigned short*)&fatinfo->fat_table[fat_offsetb];

            return table_valueb;
        case FAT_TYPE_32:
            unsigned int fat_offsetc = cluster * 4;
            //unsigned int fat_sectorc = fatinfo->first_fat_sector + (fat_offsetc / sector_size);

            unsigned int table_valuec = *(unsigned int*)&fatinfo->fat_table[fat_offsetc];
            table_valuec &= 0x0FFFFFFF;

            return table_valuec;
    }
    return 0;
}

int vfat_create_fatinfo(struct fat_info *fatinfo) {
    fatinfo->fat_size = (fatinfo->fatbs->table_size_16 == 0)? ((fat_extBS_32_t*)&(fatinfo->fatbs->extended_section))->table_size_32 : fatinfo->fatbs->table_size_16;
    fatinfo->total_sectors = (fatinfo->fatbs->total_sectors_16 == 0)? fatinfo->fatbs->total_sectors_32 : fatinfo->fatbs->total_sectors_16;
    fatinfo->root_dir_sectors = ((fatinfo->fatbs->root_entry_count * 32) + (fatinfo->fatbs->bytes_per_sector - 1)) / fatinfo->fatbs->bytes_per_sector;
    fatinfo->data_sectors = fatinfo->total_sectors - (fatinfo->fatbs->reserved_sector_count + (fatinfo->fatbs->table_count * fatinfo->fat_size) + fatinfo->root_dir_sectors);
    fatinfo->total_clusters = fatinfo->data_sectors / fatinfo->fatbs->sectors_per_cluster;
    fatinfo->first_data_sector = fatinfo->fatbs->reserved_sector_count + (fatinfo->fatbs->table_count * fatinfo->fat_size) + fatinfo->root_dir_sectors;
    fatinfo->first_fat_sector = fatinfo->fatbs->reserved_sector_count;

    if (fatinfo->fatbs->bytes_per_sector == 0) {
        // EXFAT
        fatinfo->fat_type = FAT_TYPE_EXFAT;
        return 2; // we don't support exfat
    }
    else if (fatinfo->total_clusters < 4085) {
        // FAT12
        fatinfo->fat_type = FAT_TYPE_12;
        fatinfo->root_dir_lba = fatinfo->first_data_sector - fatinfo->root_dir_sectors;
    } 
    else if (fatinfo->total_clusters < 65525) {
        // FAT16
        fatinfo->fat_type = FAT_TYPE_16;
        fatinfo->root_dir_lba = fatinfo->first_data_sector - fatinfo->root_dir_sectors;
    } 
    else {
        // FAT32
        fatinfo->fat_type = FAT_TYPE_32;
        fatinfo->root_dir_cluster = ((fat_extBS_32_t*)&(fatinfo->fatbs->extended_section))->root_cluster;
        fatinfo->root_dir_lba = ((fatinfo->root_dir_cluster - 2) * fatinfo->fatbs->sectors_per_cluster) + fatinfo->first_data_sector;
    }

    fatinfo->fentries = kmalloc(sizeof(struct fat_directory*) * 32);
    fatinfo->fentries_size = 32;
    fatinfo->fentries_loc = 0;
    return 0;
}

int vfat_read_directory(struct vfs_mount_point* mount, struct vfs_directory* dest, uint32_t cluster) {
    struct fat_info* fatinfo = (struct fat_info*)(mount->extra);
    char lfnbuf[256] = {0};
    int lfni = 0;
    unsigned char* data;
    uint32_t cluster_size = vfat_sectors_to_blocks(fatinfo->fatbs->sectors_per_cluster, mount) * mount->blockdevice->blocksize;
    data = kmalloc(cluster_size);
    //printf("%s data: %x\n", dest->name, (uint32_t)data);
    uint32_t nextcluster = vfat_read_cluster(data, cluster, mount);
    uint32_t i = 0;
    for (;;) {
        if (i+32 >= cluster_size) {
            if (nextcluster < 0x0FFFFFF8) {
                nextcluster = vfat_read_cluster(data, nextcluster, mount);
                i = 0;
            } else {
                // no more clusters to read
                break;
            }
        }

        if (data[i] == 0) {
            // no more entries to read
            break;
        }

        if (data[i] == 0xE5) {
            // unused entry
            i += 32;
            continue;
        }

        if (data[i+11] == 0x0F) {
            // lfn entry
            if (lfni+13 >= 256) { lfnbuf[lfni] = '\0'; continue; } // cap filenames
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_a[0] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_a[1] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_a[2] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_a[3] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_a[4] & 0xFF;

            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[0] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[1] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[2] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[3] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[4] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[5] & 0xFF;

            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_c[0] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_c[1] & 0xFF;

            i += 32;
            continue;
        }

        char* name;
        if (lfni) {
            int namelen = strlen(lfnbuf);
            name = kmalloc(namelen + 1);
            strncpy(name, lfnbuf, namelen);
            name[namelen] = '\0';
            memset(lfnbuf, 0, 256);
            lfni = 0;
        } else {
            name = kmalloc(13);
            memcpy(name, &data[i], 11);
            name[11] = '\0'; 
            name[12] = '\0'; 
            int isext = 0;
            for (int x = 0; x < 11; x++) {
                if (name[x] == ' ') {
                    if (isext == 2) {
                        // extension is done
                        name[x] = '\0';
                        break;
                    } else if (isext == 1) {
                        // remove the spaces and shift everything left
                        while (name[x] == ' ') {
                            for (int y = x; y < 11; y++) name[y] = name[y + 1];
                        }
                    } else {
                        // filename is done
                        for (int y = x; y < 11; y++) { 
                            if (name[y] != ' ') {
                                name[x] = '.';
                                isext = 1;
                                break;
                            }
                        }
                        if (isext == 1) continue;

                        // if we're down here, no file extension is present
                        name[x] = '\0';
                        isext = 3;
                        break;
                    }
                } else if (isext == 1) {
                    isext = 2;
                }
            }
            if (!isext) {
                // there was no space in the string, but there is a file extension
                for (int x = 11; x > 8; x--) {
                    name[x] = name[x-1];
                }
                name[8] = '.';
            } else if (isext == 3) {
                if (name[0] == '.') {
                    // ignore this entire entry
                    i += 32;
                    continue;
                }
            }
            for (int i = 0; name[i] != '\0'; i++) name[i] = tolower(name[i]);
        }
        
        if (data[i+11] & 0b00010000) {
            // this is a directory
            struct vfs_directory *newdir = (struct vfs_directory*)kmalloc(sizeof(struct vfs_directory));
            newdir->name = name;
            newdir->parent = dest;
            newdir->volume = mount;
            newdir->attributes = 0;
            
            newdir->directories = (struct vfs_directory**)kmalloc(sizeof(struct vfs_directory*) * 32);
            newdir->directories_size = 32;
            newdir->directories_len = 0;

            newdir->files = (struct vfs_file**)kmalloc(sizeof(struct vfs_file*) * 32);
            newdir->files_size = 32;
            newdir->files_len = 0;

            newdir->id = fatinfo->fentries_loc;

            if (fatinfo->fentries_size <= fatinfo->fentries_loc) {
                fatinfo->fentries_size += 32;
                fatinfo->fentries = krealloc(fatinfo->fentries, sizeof(struct fat_directory*) * fatinfo->fentries_size);
            }

            fatinfo->fentries[fatinfo->fentries_loc] = kmalloc(32);
            memcpy(fatinfo->fentries[fatinfo->fentries_loc], &data[i], 32);
            fatinfo->fentries_loc++;

            uint32_t newcluster = ((struct fat_directory*)&data[i])->cluster_lo + (((struct fat_directory*)&data[i])->cluster_hi << 16);
            vfat_read_directory(mount, newdir, newcluster);

            if (dest->directories_size <= dest->directories_len) {
                dest->directories_size += 32;
                dest->directories = krealloc(dest->directories, sizeof(struct vfs_directory*) * dest->directories_size);
            }
            dest->directories[dest->directories_len++] = newdir;
        } else {
            // this is a file
            struct vfs_file *filedescriptor = kmalloc(sizeof(struct vfs_file));

            filedescriptor->name = name;
            filedescriptor->parent = dest;
            filedescriptor->volume = mount;
            filedescriptor->size = ((struct fat_directory*)&data[i])->size;
            filedescriptor->created_time = 0;
            filedescriptor->modified_time = 0;
            filedescriptor->attributes = (data[i+11] >> 1) & 1;
            filedescriptor->open = 0;

            filedescriptor->id = fatinfo->fentries_loc;

            if (fatinfo->fentries_size <= fatinfo->fentries_loc) {
                fatinfo->fentries_size += 32;
                fatinfo->fentries = krealloc(fatinfo->fentries, sizeof(struct fat_directory*) * fatinfo->fentries_size);
            }

            fatinfo->fentries[fatinfo->fentries_loc] = kmalloc(32);
            memcpy(fatinfo->fentries[fatinfo->fentries_loc], &data[i], 32);
            fatinfo->fentries_loc++;

            if (dest->files_size <= dest->files_len) {
                dest->files_size += 32;
                dest->files = krealloc(dest->files, sizeof(struct vfs_file*) * dest->files_size);
            }
            dest->files[dest->files_len++] = filedescriptor;
        }
        i += 32;
    }
    kfree(data);
    return 0;
}

int vfat_read_root_sector(struct vfs_mount_point* mount, struct vfs_directory* dest, uint32_t sector) {
    struct fat_info* fatinfo = (struct fat_info*)(mount->extra);
    char lfnbuf[256] = {0};
    int lfni = 0;
    unsigned char* data;
    uint32_t sector_size = ((struct fat_info*)mount->extra)->fatbs->bytes_per_sector;
    data = kmalloc(sector_size);
    vfs_read_blocks(data, mount->blockdevice, vfat_sectors_to_blocks(1, mount), vfat_sector_to_lba(sector, mount));
    uint32_t i = 0;
    uint32_t sectori = 0;
    for (;;) {
        if (i+32 >= sector_size) {
            sectori++;
            vfs_read_blocks(data, mount->blockdevice, vfat_sectors_to_blocks(1, mount), vfat_sector_to_lba(sector+sectori, mount));
            i = 0;
            break;
        }

        if (data[i] == 0) {
            // no more entries to read
            break;
        }

        if (data[i] == 0xE5) {
            // unused entry
            i += 32;
            continue;
        }

        if (data[i+11] == 0x0F) {
            // lfn entry
            if (lfni+13 >= 256) { lfnbuf[lfni] = '\0'; continue; } // cap filenames
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_a[0] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_a[1] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_a[2] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_a[3] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_a[4] & 0xFF;

            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[0] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[1] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[2] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[3] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[4] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_b[5] & 0xFF;

            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_c[0] & 0xFF;
            lfnbuf[lfni++] = ((struct fat_lfn*)&data[i])->chars_c[1] & 0xFF;

            i += 32;
            continue;
        }

        char* name;
        if (lfni) {
            int namelen = strlen(lfnbuf);
            name = kmalloc(namelen + 1);
            strncpy(name, lfnbuf, namelen);
            name[namelen] = '\0';
            memset(lfnbuf, 0, 256);
            lfni = 0;
        } else {
            name = kmalloc(13);
            memcpy(name, &data[i], 11);
            name[11] = '\0'; 
            name[12] = '\0'; 
            int isext = 0;
            for (int x = 0; x < 11; x++) {
                if (name[x] == ' ') {
                    if (isext == 2) {
                        // extension is done
                        name[x] = '\0';
                        break;
                    } else if (isext == 1) {
                        // remove the spaces and shift everything left
                        while (name[x] == ' ') {
                            for (int y = x; y < 11; y++) name[y] = name[y + 1];
                        }
                    } else {
                        // filename is done
                        for (int y = x; y < 11; y++) { 
                            if (name[y] != ' ') {
                                name[x] = '.';
                                isext = 1;
                                break;
                            }
                        }
                        if (isext == 1) continue;

                        // if we're down here, no file extension is present
                        name[x] = '\0';
                        isext = 3;
                        break;
                    }
                } else if (isext == 1) {
                    isext = 2;
                }
            }
            if (!isext) {
                // there was no space in the string, but there is a file extension
                for (int x = 11; x > 8; x--) {
                    name[x] = name[x-1];
                }
                name[8] = '.';
            } else if (isext == 3) {
                if (name[0] == '.') {
                    // ignore this entire entry
                    i += 32;
                    continue;
                }
            }
            for (int i = 0; name[i] != '\0'; i++) name[i] = tolower(name[i]);
        }
        
        if (data[i+11] & 0b00010000) {
            // this is a directory
            struct vfs_directory *newdir = (struct vfs_directory*)kmalloc(sizeof(struct vfs_directory));
            newdir->name = name;
            newdir->parent = dest;
            newdir->volume = mount;
            newdir->attributes = 0;
            
            newdir->directories = (struct vfs_directory**)kmalloc(sizeof(struct vfs_directory*) * 32);
            newdir->directories_size = 32;
            newdir->directories_len = 0;

            newdir->files = (struct vfs_file**)kmalloc(sizeof(struct vfs_file*) * 32);
            newdir->files_size = 32;
            newdir->files_len = 0;

            newdir->id = fatinfo->fentries_loc;

            if (fatinfo->fentries_size <= fatinfo->fentries_loc) {
                fatinfo->fentries_size += 32;
                fatinfo->fentries = krealloc(fatinfo->fentries, sizeof(struct fat_directory*) * fatinfo->fentries_size);
            }

            fatinfo->fentries[fatinfo->fentries_loc] = kmalloc(32);
            memcpy(fatinfo->fentries[fatinfo->fentries_loc], &data[i], 32);
            fatinfo->fentries_loc++;

            uint32_t newcluster = ((struct fat_directory*)&data[i])->cluster_lo + (((struct fat_directory*)&data[i])->cluster_hi << 16);
            vfat_read_directory(mount, newdir, newcluster);

            if (dest->directories_size <= dest->directories_len) {
                dest->directories_size += 32;
                dest->directories = krealloc(dest->directories, sizeof(struct vfs_directory*) * dest->directories_size);
            }
            dest->directories[dest->directories_len++] = newdir;
        } else {
            // this is a file
            struct vfs_file *filedescriptor = kmalloc(sizeof(struct vfs_file));

            filedescriptor->name = name;
            filedescriptor->parent = dest;
            filedescriptor->volume = mount;
            filedescriptor->size = ((struct fat_directory*)&data[i])->size;
            filedescriptor->created_time = 0;
            filedescriptor->modified_time = 0;
            filedescriptor->attributes = (data[i+11] >> 1) & 1;
            filedescriptor->open = 0;

            filedescriptor->id = fatinfo->fentries_loc;

            if (fatinfo->fentries_size <= fatinfo->fentries_loc) {
                fatinfo->fentries_size += 32;
                fatinfo->fentries = krealloc(fatinfo->fentries, sizeof(struct fat_directory*) * fatinfo->fentries_size);
            }

            fatinfo->fentries[fatinfo->fentries_loc] = kmalloc(32);
            memcpy(fatinfo->fentries[fatinfo->fentries_loc], &data[i], 32);
            fatinfo->fentries_loc++;

            if (dest->files_size <= dest->files_len) {
                dest->files_size += 32;
                dest->files = krealloc(dest->files, sizeof(struct vfs_file*) * dest->files_size);
            }
            dest->files[dest->files_len++] = filedescriptor;
        }
        i += 32;
    }
    kfree(data);
    return 0;
}

int vfat_attempt_mount(struct vfs_block_device* blockdevice, const bool rw) {
    unsigned char buf[blockdevice->blocksize];
    struct vfs_mount_point *mountpoint = (struct vfs_mount_point*)kmalloc(sizeof(struct vfs_mount_point));
    struct fat_info* fatinfopointer;
    mountpoint->rw = rw;
    mountpoint->blockdevice = blockdevice;
    mountpoint->fsdriver = &vfat_vfsdrvinfo;
    if (vfs_read_blocks(buf, blockdevice, 1, 0)) {
        mountpoint->extra = kmalloc(sizeof(struct fat_info));
        fatinfopointer = (struct fat_info*)(mountpoint->extra);
        fatinfopointer->fatbs = (fat_BS_t*)kmalloc(sizeof(fat_BS_t));
        fatinfopointer->blockdevice = blockdevice;
        memcpy(fatinfopointer->fatbs, &buf, sizeof(fat_BS_t));
        if (vfat_create_fatinfo(fatinfopointer)) {
            kfree(fatinfopointer->fatbs);
            kfree(mountpoint->extra);
            return 3;
        }
        // load the FAT into memory
        uint32_t fat_block_count = vfat_sectors_to_blocks(fatinfopointer->fat_size, mountpoint);
        uint32_t fat_block_start = vfat_sector_to_lba(fatinfopointer->first_fat_sector, mountpoint);
        fatinfopointer->fat_table = kmalloc(fat_block_count * blockdevice->blocksize);
        vfs_read_blocks(fatinfopointer->fat_table, blockdevice, fat_block_count, fat_block_start);

        struct vfs_directory *root = (struct vfs_directory*)kmalloc(sizeof(struct vfs_directory));
        mountpoint->root = root;
        root->name = NULL;
        root->parent = NULL;
        root->attributes = 0;
        root->volume = mountpoint;
        
        root->directories = (struct vfs_directory**)kmalloc(sizeof(struct vfs_directory*) * 32);
        root->directories_size = 32;
        root->directories_len = 0;

        root->files = (struct vfs_file**)kmalloc(sizeof(struct vfs_file*) * 32);
        root->files_size = 32;
        root->files_len = 0;

        root->id = fatinfopointer->root_dir_cluster;

        if (fatinfopointer->fat_type == FAT_TYPE_32) {
            vfat_read_directory(mountpoint, root, fatinfopointer->root_dir_cluster);
        } else {
            vfat_read_root_sector(mountpoint, root, fatinfopointer->root_dir_lba);
        }

        blockdevice->mountpoint = mountpoint;
        
        return 0;
    } else {
        return 2;
    }
    return 1;
}

int vfat_isvalid(struct vfs_block_device *blockdevice) {
    unsigned char *buf = kmalloc(blockdevice->blocksize);
    if (!vfs_read_blocks(buf, blockdevice, 1, 0)) {
        kfree(buf);
        return 0;
    }
    if (!((buf[510] == 0x55) && (buf[511] == 0xAA))) {
        kfree(buf);
        return 0;
    }
    if (!((buf[0] == 0xEB) || (buf[0] == 0xE9))) {
        kfree(buf);
        return 0;
    }
    if (!((buf[0x36] == 'F') || (buf[0x52] == 'F'))) {
        kfree(buf);
        return 0;
    }
    kfree(buf);
    return 1;
}

ssize_t vfat_accessfile(struct vfs_block_device* blockdevice, const bool write, struct vfs_file *descriptor, unsigned char* buffer, const size_t len, const size_t loc) {
    struct fat_info* fatinfo = (struct fat_info*)(blockdevice->mountpoint->extra);

    if (write) {
        // writing
        return -1; // not yet
    } else {
        // reading
        uint32_t cluster_size = vfat_sectors_to_blocks(fatinfo->fatbs->sectors_per_cluster, blockdevice->mountpoint) * blockdevice->blocksize;
        ssize_t bytes = len;
    
        if (len >= (descriptor->size - loc)) {
            bytes = (descriptor->size - loc);
        }
        if (bytes == 0) return 0; 
        if (bytes < 0)  return -1;

        uint32_t fcluster   = fatinfo->fentries[descriptor->id]->cluster_lo + (fatinfo->fentries[descriptor->id]->cluster_hi << 16);
        uint32_t clusterloc = loc / cluster_size;
        uint32_t readsize   = round_up_integer(bytes, cluster_size);
        uint32_t clusters   = readsize / cluster_size;

        unsigned char* tempbuf = kmalloc(readsize);

        uint32_t next_cluster = fcluster;
        if (clusterloc) {
            for (int i = 0; i < clusterloc; i++) next_cluster = vfat_next_cluster(next_cluster, blockdevice->mountpoint);
        }

        for (int i = 0; i < clusters; i++) {
            next_cluster = vfat_read_cluster(&tempbuf[i * cluster_size], next_cluster, blockdevice->mountpoint);
        }

        memcpy(buffer, &tempbuf[loc % cluster_size], bytes);
        return bytes;
    }
}



void vfat_init(void) {
    vfs_register_fsdriver(&vfat_vfsdrvinfo);
}

#endif
#endif