#ifndef _GCF_H
#define _GCF_H

#include <stddef.h>
#include <stdint.h>

#define GCF_MAGIC_STRING {'G', 'C', 'F', '\0'}
#define GCF_MAGIC_LENGTH 4
#define GCF_VERSION 1

#define GCF_DEFAULT_CAPACITY 16
#define GCF_PAYLOAD_ALIGN 8

// entry flags
#define GCF_ENTRY_DELETED 0x0001
#define GCF_ENTRY_SYSTEM 0x0002

// file header flags
#define GCF_FLAG_DIRTY 0x0001

struct gcf_header {
    char magic[4];
    uint32_t version;
    uint16_t header_size;
    uint16_t entry_header_size;
    uint32_t flags;
    uint32_t entries_count;      // used slots, including tombstones
    uint32_t entries_capacity;   // allocated entry-header slots
    uint32_t payload_offset;     // absolute start of payload area
    uint32_t payload_used;       // bytes used in payload area
} __attribute__((packed));

struct gcf_entry_header {
    uint16_t flags;
    uint16_t name_length;
    uint32_t data_length;
    uint32_t name_offset;        // relative to payload_offset
    uint32_t data_offset;        // relative to payload_offset
    uint16_t name_capacity;
    uint16_t reserved;
    uint32_t data_capacity;
} __attribute__((packed));

// 0 = valid, 1 = not a gcf file, 2 = wrong version, 3 = corrupt
int gcf_is_valid(const void *data, size_t size);

// buffer returned here is malloc-owned and may be grown by the setters
void *gcf_create(size_t capacity, size_t *size);
size_t gcf_file_size(const void *data);

struct gcf_entry_header *gcf_entry_at(void *data, size_t index);
const struct gcf_entry_header *gcf_entry_at_const(const void *data, size_t index);

// returns SIZE_MAX when the entry is missing
size_t gcf_find_entry(const void *data, size_t size, const char *name, size_t name_length);

// name/data pointers are counted byte strings and are not null-terminated
const char *gcf_get_entry_name(const void *data, size_t size, size_t index);
size_t gcf_get_entry_name_length(const void *data, size_t size, size_t index);
const void *gcf_get_entry_data(const void *data, size_t size, size_t index);
size_t gcf_get_entry_data_length(const void *data, size_t size, size_t index);

// returns 0 for missing or deleted entries
uint16_t gcf_get_entry_flags(const void *data, size_t size, size_t index);

// replaces the user-visible flags; the deleted bit is not settable
int gcf_set_entry_flags(void *data, size_t size, size_t index, uint16_t flags);

// returns 1 when the entry is marked system
int gcf_entry_is_system(const void *data, size_t size, size_t index);

// the library does not enforce the system flag
int gcf_set_entry_system(void *data, size_t size, size_t index, int system_flag);

int gcf_set_entry_name(void **data, size_t *size, size_t index, const char *name, size_t name_length);
int gcf_set_entry_data(void **data, size_t *size, size_t index, const void *value, size_t data_length);
int gcf_add_entry(void **data, size_t *size, const char *name, size_t name_length, const void *value, size_t data_length, size_t *index_out);
int gcf_delete_entry(void *data, size_t size, size_t index);
int gcf_compact(void **data, size_t *size);

#endif
