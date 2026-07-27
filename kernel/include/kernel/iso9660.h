#ifndef _KERNEL_ISO9660_H
#define _KERNEL_ISO9660_H

struct iso9660_primary_volume_descriptor {
    uint8_t type;
    char identifier[5];
    uint8_t version;

    uint8_t unused1;
    char system_identifier[32];
    char volume_identifier[32];
    uint32_t unused2_l;
    uint32_t unused2_h;
    uint32_t volume_space_size;
    uint32_t volume_space_size_msb;
    char unused3[32];

    uint16_t volume_set_size;
    uint16_t volume_set_size_msb;
    uint16_t volume_sequence_number;
    uint16_t volume_sequence_number_msb;
    uint16_t logical_block_size;
    uint16_t logical_block_size_msb;

    uint32_t path_table_size;
    uint32_t path_table_size_msb;
    uint32_t path_table_loc;
    uint32_t path_table_loc_opt;
    uint32_t path_table_loc_msb;
    uint32_t path_table_loc_opt_msb;

    char root_entry[34];
    char volume_set_identifier[128];
    char publisher_identifier[128];
    char data_preparer_identifier[128];
    char application_identifier[128];
    char copyright_file_identifier[37];
    char abstract_file_identifier[37];
    char bibliographic_file_identifier[37];
    char creation_time[17];
    char modification_time[17];
    char expiration_time[17];
    char effective_time[17];

    uint8_t file_structure_version;

    uint8_t unused4;
    char application_used[512];
    char reserved[653];
} __attribute__((packed));

struct iso9660_path_table_entry {
    uint8_t length_of_directory_identifier;
    uint8_t ext_att_record_length;
    uint32_t loc_extent;
    uint16_t parent_directory;
    char directory_identifier[];
} __attribute__((packed));

struct iso9660_directory_entry {
    uint8_t entry_length;
    uint8_t ear_length;
    uint32_t file_loc;
    uint32_t file_loc_msb;
    uint32_t file_size;
    uint32_t file_size_msb;
    uint8_t time_years_since_1900;
    uint8_t time_month;
    uint8_t time_day;
    uint8_t time_hour;
    uint8_t time_minute;
    uint8_t time_second;
    signed char time_zone;
    uint8_t flags;
    uint8_t unit_size;
    uint8_t gap_size;
    uint16_t volume_sequence_number;
    uint16_t volume_sequence_number_msb;
    uint8_t file_identifier_len;
    char file_identifier[];
} __attribute__((packed));

struct iso9660_fs_info {
    unsigned char *pathtable;
    struct iso9660_path_table_entry **ptentries;
    struct vfs_directory **direntries;
    struct iso9660_primary_volume_descriptor *pvd;
    uint32_t fentries_size;
    uint32_t fentries_loc;
    struct iso9660_directory_entry **fentries;
};

void iso9660_init(void);

#endif