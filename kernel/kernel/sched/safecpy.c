#include <kernel/kernel.h>
#include <kernel/safecpy.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

extern int safecpy_asm(const unsigned char* src, unsigned char* dst);

// non-zero if [ptr, ptr+size) is entirely userspace
int safecpy_check_user(const void* ptr, size_t size) {
    uintptr_t start = (uintptr_t)ptr;
    if (start >= KERNEL_START) return 0;
    if (size > (size_t)(KERNEL_START - start)) return 0;
    return 1;
}

// dst is userspace
int safecpy_to_user(void* dstptr, const void* srcptr, size_t size) {
    if (size == 0) return 0;
    if (!safecpy_check_user(dstptr, size)) return -1;

    unsigned char* dst = (unsigned char*)dstptr;
    const unsigned char* src = (const unsigned char*)srcptr;
    for (size_t i = 0; i < size; i++)
        if (safecpy_asm(&src[i], &dst[i]) == -1)
            return -1;
    return 0;
}

// src is userspace
int safecpy_from_user(void* dstptr, const void* srcptr, size_t size) {
    if (size == 0) return 0;
    if (!safecpy_check_user(srcptr, size)) return -1;

    unsigned char* dst = (unsigned char*)dstptr;
    const unsigned char* src = (const unsigned char*)srcptr;
    for (size_t i = 0; i < size; i++)
        if (safecpy_asm(&src[i], &dst[i]) == -1)
            return -1;
    return 0;
}

// userspace string length, capped at max bytes
ssize_t safelen_max_err(const char* str, size_t max, int* err) {
    uintptr_t base = (uintptr_t)str;
    if (base >= KERNEL_START) {
        *err = -EFAULT;
        return -1;
    }

    size_t avail = (size_t)(KERNEL_START - base);
    if (max > avail) max = avail;

    for (size_t len = 0; len < max; len++) {
        unsigned char test = 0;
        if (safecpy_asm((const unsigned char*)(base + len), &test) == -1) {
            *err = -EFAULT;
            return -1;
        }
        if (!test) {
            *err = 0;
            return (ssize_t)len;
        }
    }
    *err = -ENAMETOOLONG;
    return -1; // no terminator within the limit
}

ssize_t safelen_max(const char* str, size_t max) {
    int err;
    return safelen_max_err(str, max, &err);
}

ssize_t safelen(const char* str) {
    uintptr_t base = (uintptr_t)str;
    if (base >= KERNEL_START) return -1;
    return safelen_max(str, (size_t)(KERNEL_START - base));
}
