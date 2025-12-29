#ifndef _KERNEL_RDISK_H
#define _KERNEL_RDISK_H

uint32_t rdisk_create(uint32_t blocks);
uint32_t rdisk_block(unsigned char* data, const struct vfs_block_device* blockdevice, const uint8_t write, const uint32_t index);
void rdisk_init(void);

#endif