#ifndef _KERNEL_SAFECPY_H
#define _KERNEL_SAFECPY_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

// non-zero if the whole buffer is below KERNEL_START
int safecpy_check_user(const void* ptr, size_t size);

// checked copies to/from userspace, -1 on a bad page
int safecpy_to_user(void* dstptr, const void* srcptr, size_t size);
int safecpy_from_user(void* dstptr, const void* srcptr, size_t size);

// string length, -1 if invalid or unterminated
ssize_t safelen(const char* str);
ssize_t safelen_max(const char* str, size_t max); // gives up after max bytes
// like safelen_max, but sets *err to -EFAULT or -ENAMETOOLONG
ssize_t safelen_max_err(const char* str, size_t max, int* err);

#endif
