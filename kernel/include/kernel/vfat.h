#ifndef _KERNEL_VFAT_H
#define _KERNEL_VFAT_H

#include <stdint.h>
#include <kernel/vfs.h>

#define FAT_TYPE_EXFAT 0
#define FAT_TYPE_12 12
#define FAT_TYPE_16 16
#define FAT_TYPE_32 32

typedef struct fat_extBS_32 {
    //extended fat32 stuff
    unsigned int		table_size_32;
    unsigned short		extended_flags;
    unsigned short		fat_version;
    unsigned int		root_cluster;
    unsigned short		fat_info;
    unsigned short		backup_BS_sector;
    unsigned char 		reserved_0[12];
    unsigned char		drive_number;
    unsigned char 		reserved_1;
    unsigned char		boot_signature;
    unsigned int 		volume_id;
    unsigned char		volume_label[11];
    unsigned char		fat_type_label[8];

}__attribute__((packed)) fat_extBS_32_t;

typedef struct fat_extBS_16 {
    //extended fat12 and fat16 stuff
    unsigned char		bios_drive_num;
    unsigned char		reserved1;
    unsigned char		boot_signature;
    unsigned int		volume_id;
    unsigned char		volume_label[11];
    unsigned char		fat_type_label[8];
    
}__attribute__((packed)) fat_extBS_16_t;

typedef struct fat_BS {
    unsigned char 		bootjmp[3];
    unsigned char 		oem_name[8];
    unsigned short 	    bytes_per_sector;
    unsigned char		sectors_per_cluster;
    unsigned short		reserved_sector_count;
    unsigned char		table_count;
    unsigned short		root_entry_count;
    unsigned short		total_sectors_16;
    unsigned char		media_type;
    unsigned short		table_size_16;
    unsigned short		sectors_per_track;
    unsigned short		head_side_count;
    unsigned int 		hidden_sector_count;
    unsigned int 		total_sectors_32;
    
    //this will be cast to it's specific type once the driver actually knows what type of FAT this is.
    unsigned char		extended_section[54];
    
}__attribute__((packed)) fat_BS_t;

struct fat_info {
    uint8_t fat_type;
    uint32_t total_clusters;
    uint32_t fat_size;
    uint32_t data_sectors;
    uint32_t total_sectors;
    uint32_t root_dir_sectors;
    
    fat_BS_t *fatbs;
    struct vfs_block_device* blockdevice;
};

void vfat_init(void);

#endif
