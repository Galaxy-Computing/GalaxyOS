#include <unistd.h>
#include <kernel/syscall.h>

int chdir(char* path) {
    int syscall_num = SYSCALL_CHDIR;
    int retvalue = 0;

    __asm__(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
            "m"  (path)
        : "%eax", "%ecx"
    );


    return retvalue;
}