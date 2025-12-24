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

#include <kernel/vfs.h>
#include <kernel/liballoc.h>
#include <kernel/devcfg.h>
#include <string.h>
#include <stdint.h>

#define DEFAULT_MOUNTPOINTS_LIST_SIZE 512

struct vfs_mount_point {
    char name[17]; // 16 + terminator
    uint32_t id;
    uint32_t diskdriver;
    uint8_t volumenum;
    uint32_t fsdriver;
    uint8_t rw;
};

struct vfs_mount_point *mountpoints;
uint32_t last_mount_point;
uint32_t mountpoints_size;

void vfs_init(void) {
    mountpoints = (struct vfs_mount_point*)kmalloc(sizeof(struct vfs_mount_point) * DEFAULT_MOUNTPOINTS_LIST_SIZE);
    mountpoints_size = DEFAULT_MOUNTPOINTS_LIST_SIZE;
    last_mount_point = 0;
}

struct vfs_mount_point *vfs_find_device(char* path) {
    char buf[17];
    for (size_t i = 0; i < strlen(path); i++) {
        if (path[i] == '@' || i > 16) { // paths look like drive@folder/file
            buf[i] = 0;
            break;
        }
        buf[i] = path[i];
    }
    for (uint32_t i = 0; i < last_mount_point; i++) {
        if (!strcmp(mountpoints[i].name, buf)) {
            return &mountpoints[i];
        }
    }
    return NULL;
}

struct vfs_mount_point *vfs_get_mount_info(uint32_t mountid) {
    if (last_mount_point - 1 > mountid) {
        return NULL; // this mountpoint doesn't exist
    }
    return &mountpoints[mountid];
}

uint32_t vfs_mount(struct vfs_mount_point *newmount) {
    if (last_mount_point >= mountpoints_size) {
        mountpoints = (struct vfs_mount_point*)krealloc((void*)mountpoints, sizeof(struct vfs_mount_point) * (mountpoints_size+DEFAULT_MOUNTPOINTS_LIST_SIZE));
        mountpoints_size = mountpoints_size+DEFAULT_MOUNTPOINTS_LIST_SIZE;
    }
    memcpy(&mountpoints[last_mount_point], newmount, sizeof(struct vfs_mount_point));
    mountpoints[last_mount_point].id = last_mount_point;
    return last_mount_point++;
}