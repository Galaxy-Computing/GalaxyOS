#ifndef _KERNEL_VMM_H
#define _KERNEL_VMM_H

#include <kernel/pmm.h>
#include <kernel/sched.h>

address_t vmm_get_physaddr(address_t virtualaddr);
int vmm_map_page(address_t physaddr, address_t virtualaddr, unsigned int flags, int overwrite);
void *vmm_map_pages_k(address_t *pages, int pagecount);
void *vmm_map_pages_u(address_t *pages, int pagecount, struct process *ps);
void *vmm_map_pages_loc(address_t *pages, int pagecount, address_t loc, unsigned int flags);
void vmm_map_stack(struct thread *thread);
int vmm_unmap_page(address_t virtualaddr);
void vmm_init(unsigned int pagetable);

#endif