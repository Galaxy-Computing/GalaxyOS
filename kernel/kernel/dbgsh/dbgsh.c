// Kernel Debug Shell (dbgsh.c)
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

#include <kernel/term.h>
#include <kernel/kernel.h>
#include <kernel/sched.h>
#include <kernel/pmm.h>
#include <kernel/vfs.h>
#include <kernel/liballoc.h>
#include <string.h>
#include <stdio.h>

char cmdbuf[256];

char* dbgsh_cmdline(void) {
    printf("kdbgsh> ");
    term_readline(cmdbuf, 256);
    return cmdbuf;
}

void dbgsh_main(void) {
    printf("You are now in kdbgsh.\n");
    printf("Type \"help\" for a list of commands.\n");
    int running = 1;
    while (running) {
        char* cmd = dbgsh_cmdline();
        if (!strcmp(cmd, "exit")) {
            running = 0;
        } 
        else if (!strcmp(cmd, "ver")) {
            printf("%s\n", K_VERSION);
        } 
        else if (!strcmp(cmd, "help")) {
            printf("exit       - exit kdbgsh\n");
            printf("ver        - display kernel version\n");
            printf("ps         - list processes\n");
            printf("mem        - list memory usage (mb)\n");
            printf("memk       - list memory usage (kb)\n");
            printf("lsblk      - list block devices\n");
            printf("blocktest  - test block device\n");
            printf("lsmount    - list mountpoints\n");
            printf("mount      - mount device\n");
            printf("lsroot     - list root of mountpoint\n");
        }
        else if (!strcmp(cmd, "ps")) {
            printf("id pl name\n");
            for (int i = 0; i < last_pid-1; i++) {
                printf("%i %i %s\n", i, processes[i]->privilege_level, processes[i]->name);
            }
        }
        else if (!strcmp(cmd, "memk")) {
            printf("used: %ikb\n", pmm_used() * 4);
            printf("allocated: %ikb\n", pmm_used_alloc() * 4);
            printf("available: %ikb\n", pmm_available() * 4);
        }
        else if (!strcmp(cmd, "mem")) {
            printf("used: %imb\n", pmm_used() / 256);
            printf("allocated: %imb\n", pmm_used_alloc() / 256);
            printf("available: %imb\n", pmm_available() / 256);
        }
        else if (!strcmp(cmd, "blocktest")) {
            printf("device: ");
            term_readline(cmdbuf, 256);
            struct vfs_block_device* bd = vfs_find_block_device_by_name(cmdbuf);
            if (bd == NULL) {
                printf("device doesn't exist\n");
            } else {
                char* blockbuf = (char*)kmalloc(bd->blocksize);
                uint32_t readcount = bd->block(blockbuf, bd, 0, 16);
                if (readcount) {
                    printf("%i bytes read successfully\n", readcount);
                    /*for (uint32_t i = 0; i < readcount; i++) {
                        printf("%x ", blockbuf[i]);
                    }*/
                } else {
                    printf("block read failed\n");
                }
                kfree(blockbuf);
            }
        }
        else if (!strcmp(cmd, "lsblk")) {
            if (vfs_last_blockdevice) {
                for (uint32_t i = 0; i < vfs_last_blockdevice; i++) {
                    printf(vfs_blockdevices[i].name);
                    printf("\n");
                }
            } else {
                printf("no block devices\n");
            }
        }
        else if (!strcmp(cmd, "lsmount")) {
            if (vfs_last_blockdevice) {
                int mounted = 0;
                for (uint32_t i = 0; i < vfs_last_blockdevice; i++) {
                    if (vfs_blockdevices[i].mounted) {
                        printf(vfs_blockdevices[i].name);
                        printf(" -> ");
                        printf(vfs_blockdevices[i].mountpoint->name);
                        printf("\n");
                        mounted = 1;
                    }
                }
                if (!mounted) { printf("no mounted devices\n"); }
            } else {
                printf("no block devices\n");
            }
        }
        else if (!strcmp(cmd, "mount")) {
            printf("device: ");
            term_readline(cmdbuf, 256);
            struct vfs_block_device* bd = vfs_find_block_device_by_name(cmdbuf);
            if (bd == NULL) {
                printf("device doesn't exist\n");
                continue;
            }
            struct vfs_fs_driver* fsdriver = vfs_detect_fs(bd);
            if (fsdriver == NULL) {
                printf("filesystem not supported\n");
                continue;
            }
            printf("filesystem ");
            printf(fsdriver->name);
            printf("\n");
            printf("name: ");
            char namebuf[16];
            term_readline(namebuf, 16);
            if (vfs_mount(bd, fsdriver, 0, namebuf) == NULL) {
                printf("vfs_mount() call returned NULL\n");
            }
        }
        else if (!strcmp(cmd, "lsroot")) {
            printf("path: ");
            term_readline(cmdbuf, 256);
            struct vfs_block_device* bd = vfs_find_block_device_by_path(cmdbuf);
            if (bd == NULL) {
                printf("volume doesn't exist\n");
                continue;
            }
            printf("dirs: ");
            for (uint32_t i = 0; i < bd->mountpoint->root->directories_len; i++) {
                printf(bd->mountpoint->root->directories[i]->name);
                printf(" ");
            }
            printf("\n");
            printf("files: ");
            for (uint32_t i = 0; i < bd->mountpoint->root->files_len; i++) {
                printf(bd->mountpoint->root->files[i]->name);
                printf(" ");
            }
            printf("\n");
        }
    }
}