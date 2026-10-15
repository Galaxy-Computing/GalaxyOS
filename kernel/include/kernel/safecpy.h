#ifndef _KERNEL_SAFECPY_H
#define _KERNEL_SAFECPY_H

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

int safecpy(void* restrict dstptr, const void* restrict srcptr, size_t size, int srcuser);
ssize_t safelen(const char* str, int srcuser);

#endif