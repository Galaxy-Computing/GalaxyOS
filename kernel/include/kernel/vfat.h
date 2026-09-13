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
    uint8_t  fat_type;
    uint32_t total_clusters;
    uint32_t fat_size;
    uint32_t first_data_sector;
    uint32_t data_sectors;
    uint32_t total_sectors;
    uint32_t root_dir_sectors;
    uint32_t root_dir_lba;
    uint32_t root_dir_cluster;
    uint32_t first_fat_sector;

    unsigned char* fat_table;
    
    fat_BS_t *fatbs;
    struct vfs_block_device* blockdevice;

    struct fat_directory** fentries;
    uint32_t fentries_loc;
    uint32_t fentries_size;
};

struct fat_directory {
    char     name[11];
    uint8_t  attrib;
    uint8_t  nt_reserved;
    uint8_t  time_hundredths;
    uint16_t time;
    uint16_t date;
    uint16_t a_date;
    uint16_t cluster_hi;
    uint16_t m_time;
    uint16_t m_date;
    uint16_t cluster_lo;
    uint32_t size;
} __attribute__((packed));

struct fat_lfn {
    uint8_t     index;
    uint16_t    chars_a[5]; 
    uint8_t     attrib;
    uint8_t     etype;
    uint8_t     checksum;
    uint16_t    chars_b[6];
    uint16_t    reserved;
    uint16_t    chars_c[2];
};

void vfat_init(void);

#endif
