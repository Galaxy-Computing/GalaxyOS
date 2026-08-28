#include <kernel/syscall.h>
#include <unistd.h>

int close(int fd) {
    int syscall_num = SYSCALL_CLOSE;
    ssize_t retvalue = 0;

    while (!retvalue) {
        __asm__(
            "mov %1, %%eax\n\t"
            "mov %2, %%ecx\n\t"
            "int $0x80\n\t"
            "mov %%eax, %0\n\t"
            : "=m" (retvalue)
            : "m"  (syscall_num),
              "m"  (fd)
            : "%eax", "%ecx"
        );
    }

    return retvalue;
}