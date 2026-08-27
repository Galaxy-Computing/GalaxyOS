#include <kernel/syscall.h>
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t count) {
    int syscall_num = SYSCALL_READ;
    ssize_t retvalue = 0;

    while (!retvalue) {
        __asm__(
            "mov %1, %%eax\n\t"
            "mov %2, %%ecx\n\t"
            "mov %3, %%edx\n\t"
            "mov %4, %%ebx\n\t"
            "int $0x80\n\t"
            "mov %%eax, %0\n\t"
            : "=m" (retvalue)
            : "m"  (syscall_num),
            "m"  (fd),
            "m"  (buf),
            "m"  (count)
            : "%eax", "%ecx", "%edx", "%ebx"
        );
    }

    return retvalue;
}