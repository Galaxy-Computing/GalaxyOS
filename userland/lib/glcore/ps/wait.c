#include <ps.h>
#include <kernel/syscall.h>

int psWait(int pid) {
    int syscall_num = SYSCALL_WAIT;
    int retvalue;

    __asm__(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (pid)
        : "%eax", "%ecx"
    );

    return retvalue;
}