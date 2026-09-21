#include <sysbase.h>
#include <stdint.h>
#include <kernel/syscall.h>

int sysMemUsage(struct gl_mem_usage* usagestruct) {
    int syscall_num = SYSCALL_MEMUSAGE;
    uint32_t psusage;
    uint32_t totalusage;
    uint32_t totalfree;
    uint32_t total;

    __asm__ volatile(
        "mov %4, %%eax\n\t"
        "int $0x80\n\t"
        "mov %%eax, %0\n\t"
        "mov %%ecx, %1\n\t"
        "mov %%edx, %2\n\t"
        "mov %%ebx, %3\n\t"
        : "=m" (psusage),
          "=m" (totalusage),
          "=m" (totalfree),
          "=m" (total)
        : "m"  (syscall_num)
        : "%eax", "%ecx", "%edx", "%ebx"
    );

    if (psusage == 0xffffffff) {
        return -totalusage;
    }

    usagestruct->ps_memusage = psusage;
    usagestruct->total_memusage = totalusage;
    usagestruct->total_memfree = totalfree;
    usagestruct->total_mem = total;

    return 0;
}