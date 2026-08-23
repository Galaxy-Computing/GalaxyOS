#include <ps.h>
#include <kernel/syscall.h>
#include <stdlib.h>

int psExecA(const char* path, const char* args[]) {
    int syscall_num = SYSCALL_EXEC;
    int retvalue;

    __asm__(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (path),
          "m"  (args)
        : "%eax", "%ecx", "%edx"
    );

    return retvalue;
}

int psExecS(const char* path, const char* args) {
    return -1; // not implemented
}

__attribute__((__noreturn__))
void psExit(int code) {
    exit(code);
}