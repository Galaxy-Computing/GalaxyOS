#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#if defined(__is_libk)
#include <kernel/vfs.h>
#endif

#define SYSCALL_OPEN 5

int open(const char *pathname, int flags, ...) {
    va_list args;
    va_start(args, flags);
    
    #if defined(__is_libk)
    if (flags & (O_CREAT | O_TMPFILE)) {
        mode_t mode = va_arg(args, mode_t);
        va_end(args);
        return vfs_open(pathname, flags, mode);
    }
    va_end(args);
    return vfs_open(pathname, flags);
    #else

    if (flags & (O_CREAT | O_TMPFILE)) {
        int retvalue;
        int syscall_num = SYSCALL_OPEN;

        mode_t mode = va_arg(args, mode_t);
        va_end(args);

        __asm__(
            "mov %1, %%eax\n\t"
            "mov %2, %%ecx\n\t"
            "mov %3, %%edx\n\t"
            "mov %4, %%ebx\n\t"
            "int $0x80\n\t"
            "mov %%eax, %0\n\t"
            : "=m" (retvalue) 
            : "m"  (syscall_num),
              "m"  (pathname),
              "m"  (flags),
              "m"  (mode)
            : "%eax", "%ecx", "%edx", "%ebx"
        );

        if (retvalue < 0) {
            errno = -retvalue;
            return -1;
        } 
        return retvalue;
    }

    int retvalue;
    int syscall_num = SYSCALL_OPEN;
    va_end(args);

    __asm__(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue) 
        : "m"  (syscall_num),
          "m"  (pathname),
          "m"  (flags)
        : "%eax", "%ecx", "%edx"
    );

    if (retvalue < 0) {
        errno = -retvalue;
        return -1;
    } 
    return retvalue;

    #endif
}