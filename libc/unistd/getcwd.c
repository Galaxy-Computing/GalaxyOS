#include <unistd.h>
#include <kernel/syscall.h>
#include <stddef.h>

char* getcwd(char *buf, size_t size) {
    int syscall_num = SYSCALL_GETCWD;
    char* retvalue = NULL;

    __asm__(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (buf),
          "m"  (size)
        : "%eax", "%ecx", "%edx"
    );


    return retvalue;
}