#include <ps.h>
#include <kernel/syscall.h>

int sysVersionN(char* out, int size) {
    int syscall_num = SYSCALL_GETVER;
    int retvalue;

    __asm__(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (out),
          "m"  (size)
        : "%eax", "%ecx", "%edx"
    );

    return retvalue;
}

int sysVersion(char* out) {
    sysVersionN(out, 10000); // just call it with an extremely large number so it won't hit the size limit ever
}