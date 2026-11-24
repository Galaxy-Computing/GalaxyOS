#include <sysbase.h>
#include <kernel/syscall.h>

int sysKernelPanic(char* message) {
    int syscall_num = SYSCALL_UPANIC;
    int retvalue;

    __asm__ volatile(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (message)
        : "%eax", "%ecx"
    );

    return retvalue;
}