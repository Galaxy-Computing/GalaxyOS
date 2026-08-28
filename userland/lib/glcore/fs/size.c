#include <fs.h>
#include <kernel/syscall.h>

int fsSize(int fd) {
    int syscall_num = SYSCALL_SIZE;
    int retvalue;

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

    return retvalue;
}