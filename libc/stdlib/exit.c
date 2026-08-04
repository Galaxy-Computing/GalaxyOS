#include <stdlib.h>

#define SYSCALL_EXIT 1

__attribute__((__noreturn__))
void exit(int status) {
    #if defined(__is_libk)
    abort();
    #else

    int syscall_num = SYSCALL_EXIT;

    __asm__(
        "mov %0, %%eax\n\t"
        "mov %1, %%ecx\n\t"
        "int $0x80\n\t"
        : : 
          "m"  (syscall_num),
          "m"  (status)
        : "%eax", "%ecx", "%edx"
    );

    __builtin_unreachable();

    #endif
}