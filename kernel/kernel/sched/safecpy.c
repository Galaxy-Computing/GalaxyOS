#include <kernel/kernel.h>
#include <kernel/safecpy.h>
#include <stdint.h>
#include <unistd.h>

extern int safecpy_asm(const unsigned char* src, unsigned char* dst);

int safecpy(void* restrict dstptr, const void* restrict srcptr, size_t size, int srcuser) {
    unsigned char* dst = (unsigned char*) dstptr;
    const unsigned char* src = (const unsigned char*) srcptr;
    if (srcuser) {
        if (((uintptr_t)srcptr + size) >= KERNEL_START) return -1;
    }

    for (size_t i = 0; i < size; i++)
        if (safecpy_asm(&src[i], &dst[i]) == -1)
            return -1;
    
    return 0;
}

// string passed to this function is safe to use if the return value isn't -1
ssize_t safelen(const char* str, int srcuser) {
    ssize_t len = 0;
    unsigned char test = 0xFF;
    int err = safecpy_asm(&str[len], &test);
    while (test) {
        if (err == -1) return -1;
        len++;
        err = safecpy_asm(&str[len], &test);
    }
        
    if (srcuser) {
        if (((uintptr_t)str + len) >= KERNEL_START) return -1;
    }
    return len;
}

