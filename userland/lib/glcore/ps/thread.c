#include <ps.h>
#include <kernel/syscall.h>
#include <stdio.h>

int psThreadFork(void) {
    int syscall_num = SYSCALL_TFORK;
    int retvalue;

    __asm__ volatile(
        "mov %1, %%eax\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num)
        : "%eax"
    );

    return retvalue;
}