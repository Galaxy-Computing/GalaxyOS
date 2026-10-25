#include <ps.h>
#include <kernel/syscall.h>

int psCount(void) {
    int syscall_num = SYSCALL_PSCOUNT;
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

int psList(int* list, size_t size) {
    int syscall_num = SYSCALL_PSLIST;
    int retvalue;

    __asm__ volatile(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (list),
          "m"  (size)
        : "%eax", "%ecx", "%edx"
    );

    return retvalue;
}