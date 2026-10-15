#include <ps.h>
#include <kernel/syscall.h>

int psGetEVar(const char* name, char* buf, size_t size) {
    int syscall_num = SYSCALL_GETENV;
    int retvalue;

    __asm__ volatile(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "mov %4, %%ebx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (name),
          "m"  (buf),
          "m"  (size)
        : "%eax", "%ecx", "%edx", "%ebx", "memory"
    );

    return retvalue;
}

int psSetEVar(const char* name, const char* val) {
    int syscall_num = SYSCALL_SETENV;
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
          "m"  (val)
        : "%eax", "%ecx", "%edx"
    );

    return retvalue;
}