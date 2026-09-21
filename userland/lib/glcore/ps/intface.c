#include <ps.h>
#include <errno.h>
#include <kernel/syscall.h>
#include <stdio.h>

int psInterfaceCreate(const char* name, int flags) {
    int syscall_num = SYSCALL_INTFC;
    int retvalue;

    __asm__ volatile(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (name),
          "m"  (flags)
        : "%eax", "%ecx", "%edx"
    );

    return retvalue;
}

int psInterfaceDelete(const char* name) {
    int syscall_num = SYSCALL_INTFD;
    int retvalue;

    __asm__ volatile(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (name)
        : "%eax", "%ecx"
    );

    return retvalue;
}

int psInterfaceSend(const char* name, int pid, void* buf, size_t size) {
    int syscall_num = SYSCALL_INTFS;
    int retvalue;

    __asm__ volatile(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "mov %4, %%ebx\n\t"
        "mov %5, %%esi\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (name),
          "m"  (pid),
          "m"  (buf),
          "m"  (size)
        : "%eax", "%ecx", "%edx", "%ebx", "%esi"
    );

    return retvalue;
}

int psInterfaceRecieveNB(const char* name, void* buf, size_t size, size_t* sizeout) {
    int syscall_num = SYSCALL_INTFR;
    int retvalue;

    __asm__ volatile(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "mov %4, %%ebx\n\t"
        "mov %5, %%esi\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (name),
          "m"  (buf),
          "m"  (size),
          "m"  (sizeout)
        : "%eax", "%ecx", "%edx", "%ebx", "%esi", "memory"
    );

    return retvalue;
}

int psInterfaceWait(const char* name) {
    int syscall_num = SYSCALL_INTFW;
    int retvalue;

    __asm__ volatile(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (name)
        : "%eax", "%ecx"
    );

    return retvalue;
}

int psInterfaceRecieve(const char* name, void* buf, size_t size, size_t* sizeout) {
    int retvalue = psInterfaceRecieveNB(name, buf, size, sizeout);
    if (retvalue == -EAGAIN) {
        //printf("eagain!\n");
        psInterfaceWait(name);
        //printf("%i\n", retvalue);
        retvalue = psInterfaceRecieveNB(name, buf, size, sizeout);
    }

    return retvalue;
}