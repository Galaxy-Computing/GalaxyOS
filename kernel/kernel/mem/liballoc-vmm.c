#include <kernel/liballoc.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>

#define PAGE_SIZE 0x1000

int liballoc_lock(void) {
    asm("cli");
    return 0;
}

int liballoc_unlock(void) {
    asm("sti");
    return 0;
}

void *liballoc_alloc(size_t pages) {
    address_t addr[1024];
    if (pages > 1024) {
        return NULL; // we can't allocate more than this amount of pages at a time
    }
    if (pages < 1) {
        return NULL; // can't allocate 0 pages
    }
    unsigned int i;
    for (i = 0; i < pages; i++) {
        addr[i] = pmm_alloc_page();
    }
    return (void*)vmm_map_pages_k((address_t*)addr, pages);
}

int liballoc_free(void* addr, size_t pages) {
    unsigned int i;
    for (i = 0; i < pages; i++) {
        address_t physaddr;
        physaddr = vmm_get_physaddr((address_t)addr+(i*PAGE_SIZE));
        if (!physaddr) {
            return 1;
        }
        pmm_free_page(physaddr);
        if (!vmm_unmap_page((address_t)addr+(i*PAGE_SIZE))) {
            return 1;
        }
    }
    return 0;
}