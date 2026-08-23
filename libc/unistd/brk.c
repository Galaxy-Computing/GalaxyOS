#include <kernel/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>

void* internal_brk(void *addr) {
    int syscall_num = SYSCALL_BRK;
    void *retvalue;

    __asm__(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (addr)
        : "%eax", "%ecx"
    );

    return retvalue;
}

int brk(void *addr) {
    int syscall_num = SYSCALL_BRK;
    void *retvalue = internal_brk(addr);

    if (retvalue != addr) {
        errno = ENOMEM;
        return -1;
    } 
    return 0;
}

void *sbrk(intptr_t increment) {
    void *currentbrk = internal_brk(NULL); // this syscall returns the current brk value when given NULL
    void *newbrk = (void*)((intptr_t)currentbrk + increment);
    void *changedbrk = internal_brk(newbrk);

    if (changedbrk == currentbrk) {
        errno = ENOMEM;
        return (void*)-1;
    } 
    return currentbrk;
}