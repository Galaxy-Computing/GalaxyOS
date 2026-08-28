#include <ps.h>
#include <kernel/syscall.h>

char* psGetEVar(const char* name, char* buf, size_t size) {
    int syscall_num = SYSCALL_GETENV;
    char* retvalue;

    __asm__(
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
        : "%eax", "%ecx", "%edx", "%ebx"
    );

    return retvalue;
}

int psSetEVar(const char* name, const char* val, size_t size) {
    int syscall_num = SYSCALL_SETENV;
    int retvalue;

    __asm__(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "mov %4, %%ebx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (name),
          "m"  (val),
          "m"  (size)
        : "%eax", "%ecx", "%edx", "%ebx"
    );

    return retvalue;
}