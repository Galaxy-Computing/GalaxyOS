// Virtual File System Driver (vfs.c)
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
#ifdef VFS

#include <kernel/vfs.h>
#include <kernel/liballoc.h>
#include <kernel/devreg.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define DEFAULT_BLOCKDEVICES_SIZE 512

struct vfs_block_device *vfs_blockdevices;
uint32_t vfs_last_blockdevice;
uint32_t blockdevices_size;

struct vfs_fs_driver **vfs_fsdrivers;
uint32_t vfs_fsdrivers_loc;
uint32_t vfs_fsdrivers_size;

struct vfs_block_device *vfs_find_block_device_by_path(const char* path) {
    char buf[17];
    for (size_t i = 0; i < strlen(path); i++) {
        if (path[i] == ':') { // paths look like volume:folder/file
            buf[i] = 0;
            break;
        }
        if (i > 16) {
            return NULL; // this is an invalid path
        }
        buf[i] = path[i];
    }
    for (uint32_t i = 0; i < vfs_last_blockdevice; i++) {
        if (vfs_blockdevices[i].mounted) {
            if (!strcmp(vfs_blockdevices[i].mountpoint->name, buf)) {
                return &vfs_blockdevices[i];
            }
        }
    }
    return NULL; // the referenced volume doesn't exist
}

struct vfs_block_device *vfs_get_block_device(uint32_t id) {
    if (vfs_last_blockdevice - 1 > id) {
        return NULL; // this block device doesn't exist
    }
    return &vfs_blockdevices[id];
}

struct vfs_block_device *vfs_find_block_device_by_name(const char* name) {
    for (uint32_t i = 0; i < vfs_last_blockdevice; i++) {
        if (!strcmp(vfs_blockdevices[i].name, name)) {
            return &vfs_blockdevices[i];
        }
    }
    return NULL;
}

void vfs_append_directory(struct vfs_directory* parent, struct vfs_directory* child) {
    if (parent->directories_size <= parent->directories_len) {
        parent->directories_size += 32;
        parent->directories = (struct vfs_directory**)krealloc(parent->directories, sizeof(struct vfs_directory**) * parent->directories_size);
    }
    parent->directories[parent->directories_len] = child;
    parent->directories_len++;
}

void vfs_append_directory_file(struct vfs_directory* parent, struct vfs_file* child) {
    if (parent->files_size <= parent->files_len) {
        parent->files_size += 32;
        parent->files = (struct vfs_file**)krealloc(parent->files, sizeof(struct vfs_file**) * parent->files_size);
    }
    parent->files[parent->files_len] = child;
    parent->files_len++;
}

uint32_t vfs_register_blockdevice(struct vfs_block_device *newblockdevice) {
    // need to do this weird thing or else two of these calls running at the same time would cause scary implosion of both block devices
    __sync_fetch_and_add(&vfs_last_blockdevice, 1);

    if (vfs_last_blockdevice-1 >= blockdevices_size) {
        vfs_blockdevices = (struct vfs_block_device*)krealloc((void*)vfs_blockdevices, sizeof(struct vfs_block_device) * (blockdevices_size+DEFAULT_BLOCKDEVICES_SIZE));
        blockdevices_size = blockdevices_size+DEFAULT_BLOCKDEVICES_SIZE;
    }

    memcpy(&vfs_blockdevices[vfs_last_blockdevice-1], newblockdevice, sizeof(struct vfs_block_device));
    vfs_blockdevices[vfs_last_blockdevice-1].id = vfs_last_blockdevice;
    return vfs_last_blockdevice;
}

struct vfs_mount_point *vfs_mount_direct(struct vfs_block_device *blockdevice, const struct vfs_mount_point *mp) {
    blockdevice->mountpoint = (struct vfs_mount_point*)kmalloc(sizeof(struct vfs_mount_point));
    memcpy(blockdevice->mountpoint, mp, sizeof(struct vfs_mount_point));

    return blockdevice->mountpoint;
}

struct vfs_mount_point *vfs_mount(struct vfs_block_device *blockdevice, const struct vfs_fs_driver *fsdriver, const uint8_t rw, const char* name) {
    if (!fsdriver->mount(blockdevice, rw)) {
        blockdevice->mounted = 1;
        strncpy(blockdevice->mountpoint->name, name, 16);
        blockdevice->mountpoint->name[16] = 0;
        return blockdevice->mountpoint;
    } else { return NULL; }
}

struct vfs_mount_point *vfs_mount_by_id(uint32_t blockdevice, const struct vfs_fs_driver *fsdriver, const uint8_t rw, const char* name) {
    return vfs_mount(&vfs_blockdevices[blockdevice], fsdriver, rw, name);
}

struct vfs_fs_driver *vfs_detect_fs(struct vfs_block_device *blockdevice) {
    for (uint32_t i = 0; i < vfs_fsdrivers_loc; i++) {
        if (vfs_fsdrivers[i]->isvalid(blockdevice)) {
            return vfs_fsdrivers[i];
        }
    }
    return NULL;
}

uint32_t vfs_register_fsdriver(struct vfs_fs_driver *fsdriver) {
    // need to do this weird thing or else two of these calls running at the same time could cause scary implosion of both fs driver pointers
    __sync_fetch_and_add(&vfs_fsdrivers_loc, 1);

    if (vfs_fsdrivers_loc-1 >= vfs_fsdrivers_size) {
        vfs_fsdrivers = (struct vfs_fs_driver**)krealloc((void*)vfs_fsdrivers, sizeof(struct vfs_fs_driver*) * (vfs_fsdrivers_size + 32));
        vfs_fsdrivers_size = vfs_fsdrivers_size + 32;
    }
    vfs_fsdrivers[vfs_fsdrivers_loc-1] = fsdriver;
    return vfs_fsdrivers_loc-1;
}

struct vfs_file *vfs_find_file(const char *path) {
    struct vfs_block_device *bd = vfs_find_block_device_by_path(path);

    size_t len = strlen(path) + 1;
    char *pathdup = (char*)kmalloc(len);
    memcpy(pathdup, path, len);

    char *entry = strtok(pathdup, "/");
    struct vfs_file *found = NULL;
    struct vfs_directory *searchdir = bd->mountpoint->root;
    int set = 0;

    while (entry != NULL) {
        if (!strcmp(entry, "")) { // ignore any empty entries
            set = 0;
            for (uint32_t i = 0; i < searchdir->files_len; i++) {
                if (!strcmp(searchdir->files[i]->name, entry)) {
                    found = searchdir->files[i];
                    set = 1;
                }
            }
            for (uint32_t i = 0; i < searchdir->directories_len; i++) {
                if (!strcmp(searchdir->directories[i]->name, entry)) {
                    searchdir = searchdir->directories[i];
                }
            }
        }
        entry = strtok(NULL, "/");
        if ((entry == NULL) && (set == 0)) {
            searchdir = NULL; // we didn't find the file on the last entry in the path, so it doesn't exist
        }
    }

    kfree(pathdup);
    return found;
}

struct vfs_directory *vfs_find_directory(const char *path) {
    struct vfs_block_device *bd = vfs_find_block_device_by_path(path);

    size_t len = strlen(path) + 1;
    char *pathdup = (char*)kmalloc(len);
    memcpy(pathdup, path, len);

    char *entry = strtok(pathdup, "/");
    struct vfs_directory *searchdir = bd->mountpoint->root;
    int set = 0;

    while (entry != NULL) {
        if (!strcmp(entry, "")) { // ignore any empty entries
            set = 0;
            for (uint32_t i = 0; i < searchdir->directories_len; i++) {
                if (!strcmp(searchdir->directories[i]->name, entry)) {
                    searchdir = searchdir->directories[i];
                    set = 1;
                }
            }
        }
        entry = strtok(NULL, "/");
        if ((entry == NULL) && (set == 0)) {
            searchdir = NULL; // we didn't find the directory on the last entry in the path, so it doesn't exist
        }
    }

    kfree(pathdup);
    return searchdir;
}

struct vfs_file *vfs_create_file(const char *path) {
    struct vfs_mount_point *fmount = vfs_find_block_device_by_path(path)->mountpoint;
    if (fmount == NULL) return NULL;
    return fmount->fsdriver->createfile(fmount, path);
}

struct vfs_directory *vfs_create_directory(const char *path) {
    struct vfs_mount_point *fmount = vfs_find_block_device_by_path(path)->mountpoint;
    if (fmount == NULL) return NULL;
    return fmount->fsdriver->createdirectory(fmount, path);
}

FILE *vfs_open_file(const char *path, const char *mode) {
    struct vfs_mount_point *fmount = vfs_find_block_device_by_path(path)->mountpoint;
    if (fmount == NULL) return NULL;
    struct vfs_file *vfsfile = vfs_find_file(path);

    if (!strcmp("r",mode)) {
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_READ, vfsfile);
    }
    if (!strcmp("w",mode)) {
        if (vfsfile == NULL) vfsfile = vfs_create_file(path);
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_WRITE | VFS_FILE_MODE_TRUNCATE, vfsfile);
    }
    if (!strcmp("a",mode)) {
        if (vfsfile == NULL) vfsfile = vfs_create_file(path);
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_WRITE | VFS_FILE_MODE_APPEND, vfsfile);
    }
    if (!strcmp("r+",mode)) {
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_READ | VFS_FILE_MODE_WRITE, vfsfile);
    }
    if (!strcmp("w+",mode)) {
        if (vfsfile == NULL) vfsfile = vfs_create_file(path);
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_READ | VFS_FILE_MODE_WRITE | VFS_FILE_MODE_TRUNCATE, vfsfile);
    }
    if (!strcmp("a+",mode)) {
        if (vfsfile == NULL) vfsfile = vfs_create_file(path);
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_READ | VFS_FILE_MODE_WRITE | VFS_FILE_MODE_APPEND, vfsfile);
    }

    // handle binary modes (these are treated the same way)
    if (!strcmp("rb",mode)) {
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_READ, vfsfile);
    }
    if (!strcmp("wb",mode)) {
        if (vfsfile == NULL) vfsfile = vfs_create_file(path);
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_WRITE | VFS_FILE_MODE_TRUNCATE, vfsfile);
    }
    if (!strcmp("ab",mode)) {
        if (vfsfile == NULL) vfsfile = vfs_create_file(path);
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_WRITE | VFS_FILE_MODE_APPEND, vfsfile);
    }
    if (!strcmp("rb+",mode)) {
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_READ | VFS_FILE_MODE_WRITE, vfsfile);
    }
    if (!strcmp("wb+",mode)) {
        if (vfsfile == NULL) vfsfile = vfs_create_file(path);
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_READ | VFS_FILE_MODE_WRITE | VFS_FILE_MODE_TRUNCATE, vfsfile);
    }
    if (!strcmp("ab+",mode)) {
        if (vfsfile == NULL) vfsfile = vfs_create_file(path);
        if (vfsfile == NULL) return NULL;
        return fmount->fsdriver->getfile(fmount, VFS_FILE_MODE_READ | VFS_FILE_MODE_WRITE | VFS_FILE_MODE_APPEND, vfsfile);
    }

    // invalid mode
    return NULL;
}

uint32_t vfs_read_blocks(unsigned char *dest, const struct vfs_block_device *blockdevice, const uint32_t blocks, const uint32_t index) {
    uint32_t bytesread = 0;
    for (unsigned int i = 0; i < blocks; i++) {
        bytesread += blockdevice->block(dest+(i*blockdevice->blocksize), blockdevice, 0, i+index);
    }
    return bytesread;
} 

uint32_t vfs_write_blocks(unsigned char *data, const struct vfs_block_device *blockdevice, const uint32_t blocks, const uint32_t index) {
    uint32_t byteswritten = 0;
    for (unsigned int i = 0; i < blocks; i++) {
        byteswritten += blockdevice->block(data+(i*blockdevice->blocksize), blockdevice, 1, i+index);
    }
    return byteswritten;
} 

void vfs_init(void) {
    vfs_blockdevices = (struct vfs_block_device*)kmalloc(sizeof(struct vfs_block_device) * DEFAULT_BLOCKDEVICES_SIZE);
    blockdevices_size = DEFAULT_BLOCKDEVICES_SIZE;
    vfs_last_blockdevice = 0;

    vfs_fsdrivers = (struct vfs_fs_driver**)kmalloc(sizeof(struct vfs_fsdriver*) * 32);
    vfs_fsdrivers_size = 32;
    vfs_fsdrivers_loc = 0;
}

#endif
