// Liballoc-VMM interface (liballoc-vmm.c)
// Copyright (C) 2025-2026 Skye310 (Galaxy Computing)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <kernel/liballoc.h>
#include <kernel/exception.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>
#include <kernel/kernel.h>

#define PAGE_SIZE 0x1000

int lock = 0;

int liballoc_lock(void) {
    if (kmode) {
        asm("cli");
    }
    if (!lock) {
        lock = 1;
        return 0;
    }
    return 1;
}

int liballoc_unlock(void) {
    if (kmode) {
        asm("sti");
    }
    if (lock) {
        lock = 0;
        return 0;
    }
    return 1;
}

void *liballoc_alloc(size_t pages) {
    if (pages > 1024) {
        void* start = liballoc_alloc(1024);
        for (int i = 0; i < (pages/1024)-1; i++) {
            liballoc_alloc(1024);
        }
        liballoc_alloc(pages % 1024);
        return start;
    }

    address_t addr[1024];
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