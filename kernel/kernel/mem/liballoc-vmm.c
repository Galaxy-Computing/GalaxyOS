// Liballoc-VMM interface (liballoc-vmm.c)
// Copyright (C) 2025 Skye310 (Galaxy Computing)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <kernel/liballoc.h>
#include <kernel/exception.h>
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