#ifndef _KERNEL_VMM_H
#define _KERNEL_VMM_H

#include <kernel/pmm.h>

address_t vmm_get_physaddr(address_t virtualaddr);
int vmm_map_page(address_t physaddr, address_t virtualaddr, unsigned int flags, int overwrite);
void *vmm_map_pages_k(address_t *pages, int pagecount);
int vmm_unmap_page(address_t virtualaddr);
void vmm_init(unsigned int pagetable);

#endif