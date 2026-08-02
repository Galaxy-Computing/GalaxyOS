// ISO 9660 FS Driver (iso9660.c)
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
#ifdef ISO9660

#include <kernel/liballoc.h>
#include <kernel/vfs.h>
#include <kernel/iso9660.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int iso9660_attempt_mount(struct vfs_block_device* blockdevice, const bool rw);
FILE* iso9660_getfile(struct vfs_mount_point* mountpoint, const bool write, struct vfs_file* file);
ssize_t iso9660_accessfile(struct vfs_block_device* blockdevice, const bool write, struct vfs_file *descriptor, unsigned char* buffer, const size_t len, const size_t loc);
struct vfs_file *iso9660_createfile(struct vfs_mount_point* mountpoint, const char* path);
struct vfs_directory *iso9660_createdirectory(struct vfs_mount_point* mountpoint, const char* path);
int iso9660_isvalid(struct vfs_block_device *blockdevice);

struct vfs_fs_driver iso9660_vfsdrvinfo = {
    .mount = &iso9660_attempt_mount,
    .accessfile = &iso9660_accessfile,
    .createfile = &iso9660_createfile,
    .createdirectory = &iso9660_createdirectory,
    .isvalid = &iso9660_isvalid,
    .name = "ISO9660"
};

int round_up_integer(int number, int multiple) {
    if (multiple == 0) return number;
    return ((number + multiple - 1) / multiple) * multiple;
}

// struct returned by this function should be freed after use
struct iso9660_directory_entry *iso9660_find_dir_entry(struct vfs_directory* dir, struct vfs_block_device* blockdevice) {
    struct iso9660_fs_info *fsinfo = (struct iso9660_fs_info*)blockdevice->mountpoint->extra;
    if (dir->parent == 0) {
        return (struct iso9660_directory_entry*)&fsinfo->pvd->root_entry;
    } else {
        struct iso9660_directory_entry *parententry = iso9660_find_dir_entry(dir->parent, blockdevice);
        if (parententry == NULL) return NULL;

        unsigned char *buffer = kmalloc(round_up_integer(parententry->file_size, blockdevice->blocksize));
        uint32_t buffer_blocks = (uint32_t)round_up_integer(parententry->file_size, blockdevice->blocksize) / blockdevice->blocksize;
        if (!vfs_read_blocks(buffer, blockdevice, buffer_blocks, parententry->file_loc)) return NULL;

        struct iso9660_directory_entry *found = NULL;
        uint32_t loc = 0;
        for (;;) {
            struct iso9660_directory_entry *current = (struct iso9660_directory_entry*)(buffer + loc);

            char *dirid = (char*)kmalloc(current->file_identifier_len);
            strncpy(dirid, current->file_identifier, current->file_identifier_len);
            dirid[current->file_identifier_len] = 0;

            char *dirname = strtok(dirid, ";"); // strip the version
            if (dirname == NULL) { // there's no version
                dirname = dirid;
            }

            // convert the directory name to lowercase
            for (int i = 0; dirname[i] != '\0'; i++) dirname[i] = tolower(dirname[i]);

            if (!strcmp(dirname, dir->name)) {
                found = current;
                kfree(dirid);
                break; // we found the entry, no need to continue
            }

            loc += current->entry_length;
            if (buffer[loc] == 0) {
                // handle any padding
                loc = round_up_integer(loc, blockdevice->blocksize);
            }
            
            kfree(dirid);
            if (loc >= parententry->file_size) break; // end of the entries
        }

        if (found == NULL) return NULL;

        struct iso9660_directory_entry *newdir = kmalloc(sizeof(struct iso9660_directory_entry) + found->file_identifier_len);
        memcpy(newdir, found, sizeof(struct iso9660_directory_entry) + found->file_identifier_len);

        kfree(parententry);
        kfree(buffer);
        return newdir;
    }
    return NULL;
}

// struct returned by this function should be freed after use
struct iso9660_directory_entry *iso9660_find_dir_entry_file(struct vfs_file* file, struct vfs_block_device* blockdevice) {
    //struct iso9660_fs_info *fsinfo = (struct iso9660_fs_info*)blockdevice->mountpoint->extra;

    struct iso9660_directory_entry *parententry = iso9660_find_dir_entry(file->parent, blockdevice);
    if (parententry == NULL) return NULL;

    unsigned char *buffer = kmalloc(round_up_integer(parententry->file_size, blockdevice->blocksize));
    uint32_t buffer_blocks = (uint32_t)round_up_integer(parententry->file_size, blockdevice->blocksize) / blockdevice->blocksize;
    if (!vfs_read_blocks(buffer, blockdevice, buffer_blocks, parententry->file_loc)) return NULL;

    struct iso9660_directory_entry *found = NULL;
    uint32_t loc = 0;
    for (;;) {
        struct iso9660_directory_entry *current = (struct iso9660_directory_entry*)(buffer + loc);

        char *dirid = (char*)kmalloc(current->file_identifier_len);
        strncpy(dirid, current->file_identifier, current->file_identifier_len);
        dirid[current->file_identifier_len] = 0;
        char *dirname = strtok(dirid, ";"); // strip the version

        // convert the directory name to lowercase
        for (int i = 0; dirname[i] != '\0'; i++) dirname[i] = tolower(dirname[i]);

        if (!strcmp(dirname, file->name)) {
            found = current;
            kfree(dirid);
            break; // we found the entry, no need to continue
        }

        loc += current->entry_length;
        if (buffer[loc] == 0) {
            // handle any padding
            loc = round_up_integer(loc, blockdevice->blocksize);
        }
        kfree(dirid);
        if (loc >= parententry->file_size) break; // end of the entries
    }

    if (found == NULL) return NULL;

    struct iso9660_directory_entry *newdir = kmalloc(sizeof(struct iso9660_directory_entry) + found->file_identifier_len);
    memcpy(newdir, found, sizeof(struct iso9660_directory_entry) + found->file_identifier_len);

    kfree(parententry);
    kfree(buffer);
    return newdir;
}

void iso9660_append_files(struct vfs_directory* dir, struct vfs_block_device* blockdevice) {
    struct iso9660_fs_info *fsinfo = (struct iso9660_fs_info*)blockdevice->mountpoint->extra;
    struct iso9660_directory_entry *direntry = iso9660_find_dir_entry(dir, blockdevice);

    unsigned char *buffer = kmalloc(round_up_integer(direntry->file_size, blockdevice->blocksize));
    uint32_t buffer_blocks = (uint32_t)round_up_integer(direntry->file_size, blockdevice->blocksize) / blockdevice->blocksize;
    if (!vfs_read_blocks(buffer, blockdevice, buffer_blocks, direntry->file_loc)) return;

    uint32_t loc = 0;
    for (;;) {
        struct iso9660_directory_entry *current = (struct iso9660_directory_entry*)(buffer + loc);

        if (!(current->flags & 0b10)) { // only look at files
            char *fileid = (char*)kmalloc(current->file_identifier_len);
            strncpy(fileid, current->file_identifier, current->file_identifier_len);
            fileid[current->file_identifier_len] = 0;
            char *filename = strtok(fileid, ";"); // strip the version

            int fnlen = strlen(filename);
            if (filename[fnlen-1] == '.') filename[fnlen-1] = 0; // get rid of a trailing . if the file has no extension

            // convert it to lowercase
            for (int i = 0; filename[i] != '\0'; i++) filename[i] = tolower(filename[i]);

            struct vfs_file *filedescriptor = kmalloc(sizeof(struct vfs_file));
            filedescriptor->name = filename;
            filedescriptor->parent = dir;
            filedescriptor->volume = blockdevice->mountpoint;
            filedescriptor->size = current->file_size;
            filedescriptor->created_time = 0;
            filedescriptor->modified_time = 0;
            filedescriptor->attributes = current->flags & 1;
            filedescriptor->open = 0;

            if (fsinfo->fentries_size <= fsinfo->fentries_loc) {
                fsinfo->fentries_size += 32;
                fsinfo->fentries = krealloc(fsinfo->fentries, sizeof(struct vfs_file*) * fsinfo->fentries_size);
            }

            struct iso9660_directory_entry *newdir = kmalloc(sizeof(struct iso9660_directory_entry) + current->file_identifier_len);
            memcpy(newdir, current, sizeof(struct iso9660_directory_entry) + current->file_identifier_len);

            fsinfo->fentries[fsinfo->fentries_loc] = newdir;
            filedescriptor->id = fsinfo->fentries_loc;
            
            vfs_append_directory_file(dir, filedescriptor);

            fsinfo->fentries_loc++;
        }
        
        loc += current->entry_length;
        if (buffer[loc] == 0) {
            // handle any padding
            loc = round_up_integer(loc, blockdevice->blocksize);
        }
        if (loc >= direntry->file_size) break; // end of the entries
    }
    kfree(buffer);
    kfree(direntry);
}

int iso9660_attempt_mount(struct vfs_block_device* blockdevice, const bool rw) {
    if (rw) return 3;

    struct iso9660_primary_volume_descriptor *pvd = (struct iso9660_primary_volume_descriptor*)kmalloc(blockdevice->blocksize);
    if (!vfs_read_blocks((unsigned char*)pvd, blockdevice, 1, 16)) return 1;
    if (pvd->type != 1) return 2; // why is the PVD not here

    char identifier[6];
    strncpy(identifier, pvd->identifier, 5);
    identifier[5] = 0;
    if (strcmp(identifier, "CD001")) return 2; // this is not an ISO9660 filesystem

    struct vfs_mount_point *mountpoint = (struct vfs_mount_point*)kmalloc(sizeof(struct vfs_mount_point));
    mountpoint->fsdriver = &iso9660_vfsdrvinfo;
    mountpoint->blockdevice = blockdevice;

    blockdevice->mountpoint = mountpoint;

    unsigned char *pathtable = (unsigned char*)kmalloc(round_up_integer(pvd->path_table_size, blockdevice->blocksize));
    uint32_t pathtable_blocks = (uint32_t)round_up_integer(pvd->path_table_size, blockdevice->blocksize) / blockdevice->blocksize;
    if (!vfs_read_blocks(pathtable, blockdevice, pathtable_blocks, pvd->path_table_loc)) return 1;

    // save the two pointers to the cached information
    struct iso9660_fs_info *fsinfo = (struct iso9660_fs_info*)kmalloc(sizeof(struct iso9660_fs_info));
    mountpoint->extra = (void*)fsinfo;
    fsinfo->pvd = pvd; // pvd here is 0x2badb002??
    fsinfo->pathtable = pathtable;

    struct vfs_directory *root = (struct vfs_directory*)kmalloc(sizeof(struct vfs_directory));
    mountpoint->root = root;

    // now we need to parse the path table entries
    uint32_t loc = 0;
    int ptentries_loc = 0;
    int ptentries_size = 32;
    struct iso9660_path_table_entry **ptentries = kmalloc(sizeof(struct iso9660_path_table_entry*) * ptentries_size);
    struct vfs_directory **direntries = kmalloc(sizeof(struct vfs_directory*) * ptentries_size);

    for (;;) {
        struct iso9660_path_table_entry *pte = (struct iso9660_path_table_entry*)(pathtable + loc);
        ptentries[ptentries_loc] = pte;
        
        char* dirname = (char*)kmalloc(pte->length_of_directory_identifier + 1);
        strncpy(dirname, pte->directory_identifier, pte->length_of_directory_identifier);
        dirname[pte->length_of_directory_identifier] = 0;
        if (dirname[0] == 0) { // this is the root entry
            root->name = dirname;
            root->parent = 0;
            direntries[ptentries_loc] = root;
            root->id = ptentries_loc;
            root->volume = mountpoint;
            root->files = (struct vfs_file**)kmalloc(sizeof(struct vfs_file**) * 32);
            root->directories = (struct vfs_directory**)kmalloc(sizeof(struct vfs_directory**) * 32);

            root->files_len = 0;
            root->files_size = 32;
            root->directories_len = 0;
            root->directories_size = 32;
            root->attributes = 0;
        } else {
            struct vfs_directory *dirent = (struct vfs_directory*)kmalloc(sizeof(struct vfs_directory));

            // convert the directory name to lowercase
            for (int i = 0; dirname[i] != '\0'; i++) dirname[i] = tolower(dirname[i]);

            dirent->name = dirname;
            direntries[ptentries_loc] = dirent;

            // since the path tables are in ascending order, we can assume that the parent directory has already been parsed
            // this could cause some kind of overflow if the path table was intentionally made to have a parent directory out of bounds, but i don't care enough right now
            dirent->parent = direntries[pte->parent_directory - 1]; 
            vfs_append_directory(dirent->parent, dirent);

            dirent->id = ptentries_loc;
            dirent->volume = mountpoint;
            dirent->files = (struct vfs_file**)kmalloc(sizeof(struct vfs_file**) * 32);
            dirent->directories = (struct vfs_directory**)kmalloc(sizeof(struct vfs_directory**) * 32);

            dirent->files_len = 0;
            dirent->files_size = 32;
            dirent->directories_len = 0;
            dirent->directories_size = 32;
            dirent->attributes = 0;
        }

        ptentries_loc++;
        if (ptentries_loc >= ptentries_size) { 
            ptentries_size += 32;
            ptentries = krealloc(ptentries, sizeof(struct iso9660_path_table_entry*) * ptentries_size); 
            direntries = krealloc(direntries, sizeof(struct vfs_directory*) * ptentries_size); 
        }

        loc += pte->length_of_directory_identifier + 8;
        if (pte->length_of_directory_identifier % 2) loc++; // account for padding

        if (loc >= (pvd->path_table_size - 2)) { break; } // we hit the end of the table
    }

    fsinfo->ptentries = ptentries;
    fsinfo->direntries = direntries;

    // now we need to parse the directory records to find all the files

    fsinfo->fentries = kmalloc(sizeof(struct vfs_file*) * 32);
    fsinfo->fentries_size = 32;

    for (int i = 0; i < ptentries_loc; i++) {
        iso9660_append_files(direntries[i], blockdevice);
    }

    // fs should be fully set up now
    return 0;
}

ssize_t iso9660_accessfile(struct vfs_block_device* blockdevice, const bool write, struct vfs_file *descriptor, unsigned char* buffer, const size_t len, const size_t loc) {
    if (write) return -1; // only accept reading

    struct iso9660_fs_info *fsinfo = (struct iso9660_fs_info*)blockdevice->mountpoint->extra;
    ssize_t bytes = len;
    
    if (len >= (descriptor->size - loc)) {
        bytes = (descriptor->size - loc);
    }
    
    // if this is true, either we have been asked to do nothing or the location is past the end of the file
    // either way, the application should be punished for making such a stupid call
    if (bytes == 0) return 0; 
    if (bytes < 0)  return -1; 

    uint32_t sectors;
    sectors = (bytes / blockdevice->blocksize) + 1;
    if (!(bytes % blockdevice->blocksize)) sectors--;

    uint32_t lba = fsinfo->fentries[descriptor->id]->file_loc + (loc / blockdevice->blocksize);

    unsigned char *tempbuf = kmalloc(sectors * blockdevice->blocksize);
    if (!vfs_read_blocks(tempbuf, blockdevice, sectors, lba)) {
        kfree(tempbuf);
        return -1;
    }

    memcpy(buffer, tempbuf, bytes);
    kfree(tempbuf);
    return bytes;
}

struct vfs_file *iso9660_createfile(struct vfs_mount_point* mountpoint, const char* path) {
    return NULL; // always fail, we don't support writing
}

struct vfs_directory *iso9660_createdirectory(struct vfs_mount_point* mountpoint, const char* path) {
    return NULL; // always fail, we don't support writing
}

// Returns 1 if the blockdevice contains a valid iso9660 filesystem
int iso9660_isvalid(struct vfs_block_device *blockdevice) {
    struct iso9660_primary_volume_descriptor *pvd = (struct iso9660_primary_volume_descriptor*)kmalloc(blockdevice->blocksize);
    if (!vfs_read_blocks((unsigned char*)pvd, blockdevice, 1, 16)) return 1;
    if (pvd->type != 1) return 0; // why is the PVD not here

    char identifier[6];
    strncpy(identifier, pvd->identifier, 5);
    identifier[5] = 0;
    if (strcmp(identifier, "CD001")) return 0; // this is not an ISO9660 filesystem

    return 1; // this should be an iso9660 fs
}

void iso9660_init(void) {
    vfs_register_fsdriver(&iso9660_vfsdrvinfo);
}

#endif