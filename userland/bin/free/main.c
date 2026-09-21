#include <sysbase.h>
#include <stdio.h>

struct gl_mem_usage memusage;

int main(void) {
    sysMemUsage(&memusage);
    float percentf = (float)memusage.total_memusage / (float)memusage.total_mem;
    int percent = (int)(percentf * 100);
    printf("used: %imb (%i%%)\n", memusage.total_memusage / (1024*1024), percent);
    printf("free: %imb\n", memusage.total_memfree / (1024*1024));
    printf("total: %imb\n", memusage.total_mem / (1024*1024));
}