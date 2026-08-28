#include <fs.h>
#include <kernel/syscall.h>

int fsList(const char* path, char* buf, size_t size, int dirs) {
    int syscall_num = SYSCALL_LIST;
    int retvalue;

    __asm__(
        "mov %1, %%eax\n\t"
        "mov %2, %%ecx\n\t"
        "mov %3, %%edx\n\t"
        "mov %4, %%ebx\n\t"
        "mov %5, %%esi\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        : "=m" (retvalue)
        : "m"  (syscall_num),
          "m"  (path),
          "m"  (buf),
          "m"  (size),
          "m"  (dirs)
        : "%eax", "%ecx", "%edx", "%ebx", "%esi"
    );

    return retvalue;
}
