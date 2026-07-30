#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

#define BLOCK_READ 0
#define BLOCK_WRITE 1

#define VFS_FILE_MODE_READ     0b0001
#define VFS_FILE_MODE_WRITE    0b0010
#define VFS_FILE_MODE_APPEND   0b0100
#define VFS_FILE_MODE_TRUNCATE 0b1000

struct vfs_directory;
struct vfs_file;
struct vfs_fs_driver;

struct vfs_mount_point {
    char name[17]; // 16 + terminator
    struct vfs_block_device* blockdevice;
    struct vfs_fs_driver* fsdriver;
    struct vfs_directory* root;
    void* extra; // Usable by the FS driver to point to any extra information
    uint8_t rw;
};

struct vfs_block_device {
    // function to access this block device
    // we can't use the typedef because it doesn't exist yet, and if we put the typedef above this struct then the struct wouldn't exist there
    uint32_t (*block)(unsigned char*, const struct vfs_block_device*, const uint8_t, const uint32_t); 

    uint32_t blocksize;
    // actual size is blocks*blocksize
    uint32_t blocks;

    char* name; // name of the device

    struct vfs_mount_point* mountpoint;
    uint8_t mounted; // 1 = mounted, if 0 the above field isn't used

    uint8_t ispartition; // 2 = not partition w/no partition table, 1 = has partition table, 0 = is partition
    // these fields are only used if ispartition is 0, if they aren't used these are free for the device driver to use for any purpose
    uint32_t parentid;
    struct vfs_block_device* parent;

    uint8_t isremovable; // 1 = is removable media
    uint8_t hasmedia;    // 1 = media is inserted, 0 = no media

    void* extraa; // Usable by the device driver to point to any extra information
    void* extrab; // Usable by the partition table driver to point to any extra information

    uint32_t id;
};

typedef uint32_t (*vfs_block)(unsigned char*, const struct vfs_block_device*, const uint8_t, const uint32_t);
typedef int (*vfs_fs_mount)(struct vfs_block_device*, const bool);
typedef ssize_t (*vfs_fs_accessfile)(struct vfs_block_device*, const bool, struct vfs_file*, unsigned char*, const size_t, const size_t);
typedef struct vfs_file *(*vfs_fs_createfile)(struct vfs_mount_point*, const char*);
typedef struct vfs_directory *(*vfs_fs_createdirectory)(struct vfs_mount_point*, const char*);
typedef int (*vfs_fs_isvalid)(struct vfs_block_device*);

struct vfs_fs_driver {
    vfs_fs_mount mount;
    vfs_fs_accessfile accessfile;
    vfs_fs_createfile createfile;
    vfs_fs_createdirectory createdirectory;
    vfs_fs_isvalid isvalid;
    char* name;
};

struct vfs_directory {
    char* name;
    struct vfs_directory* parent; // this is set to 0 if this is the root of the drive
    struct vfs_mount_point* volume;
    struct vfs_file** files;
    struct vfs_directory** directories;
    uint32_t files_len;
    uint32_t files_size;
    uint32_t directories_len;
    uint32_t directories_size;
    uint32_t id;
    uint8_t attributes;
};

struct vfs_file {
    char* name;
    struct vfs_directory* parent;
    struct vfs_mount_point* volume;
    uint32_t id;
    uint32_t size;
    uint32_t created_time;
    uint32_t modified_time;
    uint8_t attributes; // 1 = hidden
    bool open; // true = open
};

struct vfs_file_open {
    struct vfs_file* file;
    struct vfs_fs_driver* fsdriver;
    uint32_t loc;
    int flags;
    int id;
};

extern struct vfs_block_device *vfs_blockdevices;
extern uint32_t vfs_last_blockdevice;

void vfs_init(void);
struct vfs_block_device *vfs_find_block_device_by_path(const char* path);
struct vfs_block_device *vfs_find_block_device_by_name(const char* name);
void vfs_append_directory(struct vfs_directory* parent, struct vfs_directory* child);
void vfs_append_directory_file(struct vfs_directory* parent, struct vfs_file* child);

struct vfs_file *vfs_find_file(const char *path);
struct vfs_directory *vfs_find_directory(const char *path);

struct vfs_mount_point *vfs_get_mount_info(uint32_t mountid);
struct vfs_mount_point *vfs_mount_direct(struct vfs_block_device *blockdevice, const struct vfs_mount_point *mp);
FILE *vfs_open_file(const char *filename, const char *mode);
uint32_t vfs_register_blockdevice(struct vfs_block_device *newblockdevice);
uint32_t vfs_register_fsdriver(struct vfs_fs_driver *fsdriver);
struct vfs_fs_driver *vfs_detect_fs(struct vfs_block_device *blockdevice);
struct vfs_mount_point *vfs_mount(struct vfs_block_device *blockdevice, const struct vfs_fs_driver *fsdriver, const bool rw, const char* name);

uint32_t vfs_read_blocks(unsigned char *dest, const struct vfs_block_device* blockdevice, const uint32_t blocks, const uint32_t index);
uint32_t vfs_write_blocks(unsigned char *data, const struct vfs_block_device* blockdevice, const uint32_t blocks, const uint32_t index);

// vfs implementations of the system calls
int vfs_open(const char *path, int flags);
int vfs_close(int fd);
ssize_t vfs_read(int fd, void *buf, size_t count);
ssize_t vfs_write(int fd, void *buf, size_t count);
int vfs_fstat(int fd, struct stat *statbuf);

#endif