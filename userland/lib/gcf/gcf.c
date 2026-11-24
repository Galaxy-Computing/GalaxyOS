#include "gcf.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static const char gcf_magic[GCF_MAGIC_LENGTH] = GCF_MAGIC_STRING;

_Static_assert(sizeof(struct gcf_header) == 32, "unexpected gcf header size");
_Static_assert(sizeof(struct gcf_entry_header) == 24, "unexpected gcf entry header size");

static int align_up(size_t value, size_t align, size_t *out) {
    if (value > SIZE_MAX - (align - 1)) { return -1; }
    *out = (value + align - 1) & ~(align - 1);
    return 0;
}

static int gcf_header_ok(const void *data, size_t size) {
    if (data == NULL || size < GCF_MAGIC_LENGTH) { return 1; }
    if (memcmp(data, gcf_magic, GCF_MAGIC_LENGTH) != 0) { return 1; }
    if (size < sizeof(struct gcf_header)) { return 3; }

    const struct gcf_header *h = data;
    if (h->version != GCF_VERSION) { return 2; }
    if (h->header_size < sizeof(struct gcf_header)) { return 3; }
    if (h->entry_header_size < sizeof(struct gcf_entry_header)) { return 3; }
    if (h->entries_count > h->entries_capacity) { return 3; }
    if (h->header_size > h->payload_offset) { return 3; }
    if (h->payload_offset > size) { return 3; }
    if (h->payload_used > size - h->payload_offset) { return 3; }

    if (h->entries_capacity > (SIZE_MAX - h->header_size) / h->entry_header_size) { return 3; }
    size_t table_end = (size_t)h->header_size + (size_t)h->entries_capacity * h->entry_header_size;
    if (table_end > h->payload_offset) { return 3; }

    return 0;
}

int gcf_is_valid(const void *data, size_t size) {
    int ret = gcf_header_ok(data, size);
    if (ret != 0) { return ret; }

    const struct gcf_header *h = data;
    for (uint32_t i = 0; i < h->entries_count; i++) {
        const struct gcf_entry_header *entry = gcf_entry_at_const(data, i);
        if (entry->flags & GCF_ENTRY_DELETED) { continue; }

        size_t name_capacity = entry->name_capacity;
        size_t data_capacity = entry->data_capacity;
        if (name_capacity < entry->name_length) { name_capacity = entry->name_length; }
        if (data_capacity < entry->data_length) { data_capacity = entry->data_length; }

        if (entry->name_offset > h->payload_used) { return 3; }
        if (name_capacity > h->payload_used - entry->name_offset) { return 3; }
        if (entry->data_offset > h->payload_used) { return 3; }
        if (data_capacity > h->payload_used - entry->data_offset) { return 3; }
    }

    return 0;
}

struct gcf_entry_header *gcf_entry_at(void *data, size_t index) {
    struct gcf_header *h = data;
    return (struct gcf_entry_header*)((char*)data + h->header_size + index * (size_t)h->entry_header_size);
}

const struct gcf_entry_header *gcf_entry_at_const(const void *data, size_t index) {
    const struct gcf_header *h = data;
    return (const struct gcf_entry_header*)((const char*)data + h->header_size + index * (size_t)h->entry_header_size);
}

void *gcf_create(size_t capacity, size_t *size) {
    if (capacity == 0) { capacity = 1; }
    if (capacity > UINT32_MAX) { errno = EINVAL; return NULL; }
    if (capacity > (SIZE_MAX - sizeof(struct gcf_header)) / sizeof(struct gcf_entry_header)) {
        errno = EOVERFLOW;
        return NULL;
    }

    size_t table_end = sizeof(struct gcf_header) + capacity * sizeof(struct gcf_entry_header);
    size_t payload_offset;
    if (align_up(table_end, GCF_PAYLOAD_ALIGN, &payload_offset) < 0
            || payload_offset > UINT32_MAX) {
        errno = EOVERFLOW;
        return NULL;
    }

    void *data = calloc(1, payload_offset);
    if (data == NULL) { return NULL; }

    struct gcf_header *h = data;
    memcpy(h->magic, gcf_magic, GCF_MAGIC_LENGTH);
    h->version = GCF_VERSION;
    h->header_size = (uint16_t)sizeof(struct gcf_header);
    h->entry_header_size = (uint16_t)sizeof(struct gcf_entry_header);
    h->flags = 0;
    h->entries_count = 0;
    h->entries_capacity = (uint32_t)capacity;
    h->payload_offset = (uint32_t)payload_offset;
    h->payload_used = 0;

    if (size != NULL) { *size = payload_offset; }
    return data;
}

size_t gcf_file_size(const void *data) {
    if (data == NULL) { return 0; }
    const struct gcf_header *h = data;
    return (size_t)h->payload_offset + h->payload_used;
}

size_t gcf_find_entry(const void *data, size_t size, const char *name, size_t name_length) {
    if (name == NULL) { return SIZE_MAX; }
    if (gcf_is_valid(data, size) != 0) { return SIZE_MAX; }

    const struct gcf_header *h = data;
    for (uint32_t i = 0; i < h->entries_count; i++) {
        const struct gcf_entry_header *entry = gcf_entry_at_const(data, i);
        if (entry->flags & GCF_ENTRY_DELETED) { continue; }
        if (entry->name_length != name_length) { continue; }
        if (name_length == 0) { return i; }
        if (memcmp((const char*)data + h->payload_offset + entry->name_offset, name, name_length) == 0) {
            return i;
        }
    }

    return SIZE_MAX;
}

const char *gcf_get_entry_name(const void *data, size_t size, size_t index) {
    if (gcf_header_ok(data, size) != 0) { return NULL; }

    const struct gcf_header *h = data;
    if (index >= h->entries_count) { return NULL; }

    const struct gcf_entry_header *entry = gcf_entry_at_const(data, index);
    if ((entry->flags & GCF_ENTRY_DELETED) || entry->name_length == 0) { return NULL; }
    if (entry->name_offset > h->payload_used) { return NULL; }
    if (entry->name_length > h->payload_used - entry->name_offset) { return NULL; }

    return (const char*)data + h->payload_offset + entry->name_offset;
}

size_t gcf_get_entry_name_length(const void *data, size_t size, size_t index) {
    if (gcf_header_ok(data, size) != 0) { return 0; }

    const struct gcf_header *h = data;
    if (index >= h->entries_count) { return 0; }

    const struct gcf_entry_header *entry = gcf_entry_at_const(data, index);
    if (entry->flags & GCF_ENTRY_DELETED) { return 0; }
    if (entry->name_offset > h->payload_used) { return 0; }
    if (entry->name_length > h->payload_used - entry->name_offset) { return 0; }

    return entry->name_length;
}

const void *gcf_get_entry_data(const void *data, size_t size, size_t index) {
    if (gcf_header_ok(data, size) != 0) { return NULL; }

    const struct gcf_header *h = data;
    if (index >= h->entries_count) { return NULL; }

    const struct gcf_entry_header *entry = gcf_entry_at_const(data, index);
    if ((entry->flags & GCF_ENTRY_DELETED) || entry->data_length == 0) { return NULL; }
    if (entry->data_offset > h->payload_used) { return NULL; }
    if (entry->data_length > h->payload_used - entry->data_offset) { return NULL; }

    return (const void*)((const char*)data + h->payload_offset + entry->data_offset);
}

size_t gcf_get_entry_data_length(const void *data, size_t size, size_t index) {
    if (gcf_header_ok(data, size) != 0) { return 0; }

    const struct gcf_header *h = data;
    if (index >= h->entries_count) { return 0; }

    const struct gcf_entry_header *entry = gcf_entry_at_const(data, index);
    if (entry->flags & GCF_ENTRY_DELETED) { return 0; }
    if (entry->data_offset > h->payload_used) { return 0; }
    if (entry->data_length > h->payload_used - entry->data_offset) { return 0; }

    return entry->data_length;
}

uint16_t gcf_get_entry_flags(const void *data, size_t size, size_t index) {
    if (gcf_header_ok(data, size) != 0) { return 0; }

    const struct gcf_header *h = data;
    if (index >= h->entries_count) { return 0; }

    const struct gcf_entry_header *entry = gcf_entry_at_const(data, index);
    if (entry->flags & GCF_ENTRY_DELETED) { return 0; }
    return entry->flags;
}

int gcf_set_entry_flags(void *data, size_t size, size_t index, uint16_t flags) {
    if (gcf_is_valid(data, size) != 0) { errno = EINVAL; return -1; }

    struct gcf_header *h = data;
    if (index >= h->entries_count) { errno = ENOENT; return -1; }

    struct gcf_entry_header *entry = gcf_entry_at(data, index);
    if (entry->flags & GCF_ENTRY_DELETED) { errno = ENOENT; return -1; }

    entry->flags = (uint16_t)(flags & (uint16_t)~GCF_ENTRY_DELETED);
    return 0;
}

int gcf_entry_is_system(const void *data, size_t size, size_t index) {
    return (gcf_get_entry_flags(data, size, index) & GCF_ENTRY_SYSTEM) != 0;
}

int gcf_set_entry_system(void *data, size_t size, size_t index, int system_flag) {
    uint16_t flags = gcf_get_entry_flags(data, size, index);
    if (system_flag) {
        flags |= GCF_ENTRY_SYSTEM;
    } else {
        flags = (uint16_t)(flags & (uint16_t)~GCF_ENTRY_SYSTEM);
    }
    return gcf_set_entry_flags(data, size, index, flags);
}

static int gcf_alloc_block(void **data, size_t *size, size_t length, size_t max_capacity, size_t *offset_out, uint32_t *capacity_out) {
    if (length == 0) {
        *offset_out = 0;
        *capacity_out = 0;
        return 0;
    }
    if (length > max_capacity) { errno = E2BIG; return -1; }

    size_t capacity;
    if (align_up(length, GCF_PAYLOAD_ALIGN, &capacity) < 0) {
        errno = EOVERFLOW;
        return -1;
    }
    if (capacity > max_capacity) { capacity = max_capacity; }

    struct gcf_header *h = *data;
    if (h->payload_used > UINT32_MAX - capacity) {
        errno = EOVERFLOW;
        return -1;
    }

    size_t offset = h->payload_used;
    size_t absolute = (size_t)h->payload_offset + offset;
    if (absolute > SIZE_MAX - capacity) {
        errno = EOVERFLOW;
        return -1;
    }
    size_t needed = absolute + capacity;

    if (needed > *size) {
        void *grown = realloc(*data, needed);
        if (grown == NULL) { return -1; }
        *data = grown;
        *size = needed;
        h = *data;
    }

    memset((char*)*data + absolute, 0, capacity);
    h->payload_used = (uint32_t)(offset + capacity);
    *offset_out = offset;
    *capacity_out = (uint32_t)capacity;
    return 0;
}

static int gcf_grow_entries(void **data, size_t *size) {
    struct gcf_header *h = *data;
    uint32_t old_capacity = h->entries_capacity;
    if (old_capacity == UINT32_MAX) { errno = EOVERFLOW; return -1; }

    uint32_t new_capacity = old_capacity ? old_capacity * 2 : 1;
    if (new_capacity < old_capacity) { new_capacity = UINT32_MAX; }

    if ((size_t)new_capacity > (SIZE_MAX - h->header_size) / h->entry_header_size) {
        errno = EOVERFLOW;
        return -1;
    }

    size_t table_end = (size_t)h->header_size
                     + (size_t)new_capacity * h->entry_header_size;
    size_t new_payload_offset;
    if (align_up(table_end, GCF_PAYLOAD_ALIGN, &new_payload_offset) < 0 || new_payload_offset > UINT32_MAX) {
        errno = EOVERFLOW;
        return -1;
    }
    if (h->payload_used > SIZE_MAX - new_payload_offset) {
        errno = EOVERFLOW;
        return -1;
    }
    size_t new_size = new_payload_offset + h->payload_used;

    size_t old_payload_offset = h->payload_offset;
    uint32_t payload_used = h->payload_used;
    uint32_t entries_count = h->entries_count;

    if (new_size > *size) {
        void *grown = realloc(*data, new_size);
        if (grown == NULL) { return -1; }
        *data = grown;
        *size = new_size;
    }

    h = *data;
    memmove((char*)*data + new_payload_offset, (char*)*data + old_payload_offset, payload_used);

    size_t zero_start = (size_t)h->header_size
                      + (size_t)entries_count * h->entry_header_size;
    if (new_payload_offset > zero_start) {
        memset((char*)*data + zero_start, 0, new_payload_offset - zero_start);
    }

    h->entries_capacity = new_capacity;
    h->payload_offset = (uint32_t)new_payload_offset;
    return 0;
}

int gcf_set_entry_name(void **data, size_t *size, size_t index, const char *name, size_t name_length) {
    if (data == NULL || size == NULL) { errno = EINVAL; return -1; }
    if (name == NULL && name_length != 0) { errno = EINVAL; return -1; }
    if (gcf_is_valid(*data, *size) != 0) { errno = EINVAL; return -1; }
    if (name_length > UINT16_MAX) { errno = ENAMETOOLONG; return -1; }

    struct gcf_header *h = *data;
    if (index >= h->entries_count) { errno = ENOENT; return -1; }

    struct gcf_entry_header *entry = gcf_entry_at(*data, index);
    if (entry->flags & GCF_ENTRY_DELETED) { errno = ENOENT; return -1; }

    size_t capacity = entry->name_capacity;
    if (capacity < entry->name_length) { capacity = entry->name_length; }

    if (name_length <= capacity) {
        if (name_length > 0) {
            memcpy((char*)*data + h->payload_offset + entry->name_offset, name, name_length);
        }
        entry->name_length = (uint16_t)name_length;
        return 0;
    }

    size_t offset;
    uint32_t new_capacity;
    if (gcf_alloc_block(data, size, name_length, UINT16_MAX, &offset, &new_capacity) < 0) {
        return -1;
    }

    h = *data;
    entry = gcf_entry_at(*data, index);
    if (name_length > 0) {
        memcpy((char*)*data + h->payload_offset + offset, name, name_length);
    }
    entry->name_offset = (uint32_t)offset;
    entry->name_capacity = (uint16_t)new_capacity;
    entry->name_length = (uint16_t)name_length;
    return 0;
}

int gcf_set_entry_data(void **data, size_t *size, size_t index, const void *value, size_t data_length) {
    if (data == NULL || size == NULL) { errno = EINVAL; return -1; }
    if (value == NULL && data_length != 0) { errno = EINVAL; return -1; }
    if (gcf_is_valid(*data, *size) != 0) { errno = EINVAL; return -1; }
    if (data_length > UINT32_MAX) { errno = E2BIG; return -1; }

    struct gcf_header *h = *data;
    if (index >= h->entries_count) { errno = ENOENT; return -1; }

    struct gcf_entry_header *entry = gcf_entry_at(*data, index);
    if (entry->flags & GCF_ENTRY_DELETED) { errno = ENOENT; return -1; }

    size_t capacity = entry->data_capacity;
    if (capacity < entry->data_length) { capacity = entry->data_length; }

    if (data_length <= capacity) {
        if (data_length > 0) {
            memcpy((char*)*data + h->payload_offset + entry->data_offset, value, data_length);
        }
        entry->data_length = (uint32_t)data_length;
        return 0;
    }

    size_t offset;
    uint32_t new_capacity;
    if (gcf_alloc_block(data, size, data_length, UINT32_MAX, &offset, &new_capacity) < 0) {
        return -1;
    }

    h = *data;
    entry = gcf_entry_at(*data, index);
    if (data_length > 0) {
        memcpy((char*)*data + h->payload_offset + offset, value, data_length);
    }
    entry->data_offset = (uint32_t)offset;
    entry->data_capacity = new_capacity;
    entry->data_length = (uint32_t)data_length;
    return 0;
}

int gcf_add_entry(void **data, size_t *size, const char *name, size_t name_length, const void *value, size_t data_length, size_t *index_out) {
    if (data == NULL || size == NULL) { errno = EINVAL; return -1; }
    if (name == NULL && name_length != 0) { errno = EINVAL; return -1; }
    if (value == NULL && data_length != 0) { errno = EINVAL; return -1; }
    if (gcf_is_valid(*data, *size) != 0) { errno = EINVAL; return -1; }
    if (name_length > UINT16_MAX) { errno = ENAMETOOLONG; return -1; }
    if (data_length > UINT32_MAX) { errno = E2BIG; return -1; }

    struct gcf_header *h = *data;
    size_t index = SIZE_MAX;
    for (uint32_t i = 0; i < h->entries_count; i++) {
        struct gcf_entry_header *entry = gcf_entry_at(*data, i);
        if (entry->flags & GCF_ENTRY_DELETED) {
            index = i;
            break;
        }
    }

    int is_new = 0;
    if (index == SIZE_MAX) {
        if (h->entries_count == h->entries_capacity) {
            if (gcf_grow_entries(data, size) < 0) { return -1; }
            h = *data;
        }
        index = h->entries_count;
        is_new = 1;
    }

    size_t name_offset = 0;
    size_t data_offset = 0;
    uint32_t name_capacity = 0;
    uint32_t data_capacity = 0;

    if (name_length > 0) {
        if (gcf_alloc_block(data, size, name_length, UINT16_MAX, &name_offset, &name_capacity) < 0) {
            return -1;
        }
    }
    if (data_length > 0) {
        if (gcf_alloc_block(data, size, data_length, UINT32_MAX, &data_offset, &data_capacity) < 0) {
            return -1;
        }
    }

    h = *data;
    struct gcf_entry_header *entry = gcf_entry_at(*data, index);
    memset(entry, 0, sizeof(*entry));
    entry->flags = 0;
    entry->name_offset = (uint32_t)name_offset;
    entry->name_length = (uint16_t)name_length;
    entry->name_capacity = (uint16_t)name_capacity;
    entry->data_offset = (uint32_t)data_offset;
    entry->data_length = (uint32_t)data_length;
    entry->data_capacity = data_capacity;

    if (name_length > 0) {
        memcpy((char*)*data + h->payload_offset + name_offset, name, name_length);
    }
    if (data_length > 0) {
        memcpy((char*)*data + h->payload_offset + data_offset, value, data_length);
    }

    if (is_new) { h->entries_count = (uint32_t)(index + 1); }
    if (index_out != NULL) { *index_out = index; }
    return 0;
}

int gcf_delete_entry(void *data, size_t size, size_t index) {
    if (gcf_is_valid(data, size) != 0) { errno = EINVAL; return -1; }

    struct gcf_header *h = data;
    if (index >= h->entries_count) { errno = ENOENT; return -1; }

    struct gcf_entry_header *entry = gcf_entry_at(data, index);
    if (entry->flags & GCF_ENTRY_DELETED) { errno = ENOENT; return -1; }

    entry->flags |= GCF_ENTRY_DELETED;
    entry->name_length = 0;
    entry->data_length = 0;
    entry->name_capacity = 0;
    entry->data_capacity = 0;
    entry->name_offset = 0;
    entry->data_offset = 0;

    while (h->entries_count > 0) {
        struct gcf_entry_header *last = gcf_entry_at(data, h->entries_count - 1);
        if ((last->flags & GCF_ENTRY_DELETED) == 0) { break; }
        memset(last, 0, sizeof(*last));
        h->entries_count--;
    }

    return 0;
}

int gcf_compact(void **data, size_t *size) {
    if (data == NULL || size == NULL || gcf_is_valid(*data, *size) != 0) {
        errno = EINVAL;
        return -1;
    }

    struct gcf_header *old = *data;
    size_t live = 0;
    for (uint32_t i = 0; i < old->entries_count; i++) {
        struct gcf_entry_header *entry = gcf_entry_at(*data, i);
        if ((entry->flags & GCF_ENTRY_DELETED) == 0) { live++; }
    }

    size_t new_size;
    void *fresh = gcf_create(live > 0 ? live : 1, &new_size);
    if (fresh == NULL) { return -1; }

    for (uint32_t i = 0; i < old->entries_count; i++) {
        struct gcf_entry_header *entry = gcf_entry_at(*data, i);
        if (entry->flags & GCF_ENTRY_DELETED) { continue; }

        const char *name = (const char*)*data + old->payload_offset + entry->name_offset;
        const void *value = (const char*)*data + old->payload_offset + entry->data_offset;
        size_t new_index;
        if (gcf_add_entry(&fresh, &new_size, name, entry->name_length, value, entry->data_length, &new_index) < 0) {
            free(fresh);
            return -1;
        }
        struct gcf_entry_header *copied = gcf_entry_at(fresh, new_index);
        copied->flags = (uint16_t)(entry->flags & (uint16_t)~GCF_ENTRY_DELETED);
        copied->reserved = entry->reserved;
    }

    free(*data);
    *data = fresh;
    *size = new_size;
    return 0;
}
