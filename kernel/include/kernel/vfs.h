#ifndef _KERNEL_VFS_H
#define _KERNEL_VFS_H

#include <stdint.h>
#include <stdio.h>

#define BLOCK_READ 0
#define BLOCK_WRITE 1

struct vfs_mount_point {
    char name[17]; // 16 + terminator
    uint32_t blockdeviceid;
    struct vfs_device_driver* devicedriver;
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
typedef int (*vfs_fs_mount)(struct vfs_block_device*, const uint8_t);

struct vfs_fs_driver {
    vfs_fs_mount mount;
};

extern struct vfs_block_device *vfs_blockdevices;
extern uint32_t vfs_last_blockdevice;

void vfs_init(void);
struct vfs_block_device *vfs_find_block_device_by_path(const char* path);
struct vfs_block_device *vfs_find_block_device_by_name(const char* name);
struct vfs_mount_point *vfs_get_mount_info(uint32_t mountid);
struct vfs_mount_point *vfs_mount_direct(struct vfs_block_device *blockdevice, const struct vfs_mount_point *mp);
FILE *vfs_get_file(const char *filename, const char *mode);
uint32_t vfs_register_blockdevice(struct vfs_block_device *newblockdevice);

uint32_t vfs_read_blocks(unsigned char *dest, const struct vfs_block_device* blockdevice, const uint32_t blocks, const uint32_t index);
uint32_t vfs_write_blocks(unsigned char *data, const struct vfs_block_device* blockdevice, const uint32_t blocks, const uint32_t index);

#endif