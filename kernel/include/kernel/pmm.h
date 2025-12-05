#ifndef _KERNEL_PMM_H
#define _KERNEL_PMM_H

#include <kernel/multiboot.h>

#define MAX_PAGES 1024*1024
typedef uintptr_t address_t; // uintptr_t is C99 and defined in stdint.h

address_t pmm_available(void);
address_t pmm_alloc_page(void);
void pmm_free_page(address_t paddr);
void pmm_init(multiboot_info_t* mbd);

#endif