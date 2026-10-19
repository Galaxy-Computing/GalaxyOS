#include <kernel/syscall.h>

int sysSetPowerState(int state) {
    int syscall_num = SYSCALL_SPSTATE;
    int retvalue;

    __asm__ volatile(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (state)
        : "%eax", "%ecx"
    );

    return retvalue;
}