// Virtual Memory Manager (vmm.c)
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


#include <stdint.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/liballoc.h>
#include <kernel/exception.h>
#include <kernel/sched.h>
#include <stddef.h>
#include <string.h>

#define PAGE_DIRECTORY_ADDR 0xFFFFF000
#define PAGE_TABLE_ADDR 0xFFC00000
#define PAGE_SIZE 0x1000

address_t end_mapped_kmemory = 0xC0C00000;
address_t page_table_physaddr;

static inline void __invlpg(unsigned int addr) {
   asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

address_t vmm_get_physaddr(address_t virtualaddr) {
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

    unsigned long *pd = (unsigned long *)PAGE_DIRECTORY_ADDR;
    if ((pd[pdindex] & 1) == 0) {
		return 0; // this isn't present
	}

    unsigned long *pt = ((unsigned long *)PAGE_TABLE_ADDR) + (0x400 * pdindex);
    if ((pt[ptindex] & 1) == 0) {
		return 0; // this isn't present
	}

    return (address_t)((pt[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
}

int vmm_map_page(address_t physaddr, address_t virtualaddr, unsigned int flags, int overwrite) {
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

    unsigned long *pd = (unsigned long *)PAGE_DIRECTORY_ADDR;
    unsigned long *pt = ((unsigned long *)PAGE_TABLE_ADDR) + (0x400 * pdindex);
    unsigned long ptphys;

	if ((pd[pdindex] & 1) == 0) {
		// this isn't present, create a new one
        // we should only be here for user pages
        void* newptaddr = liballoc_alloc(1); // get a page for the page table
        memset(newptaddr, 0, PAGE_SIZE); // zero it out, we don't want any random data that was in there earlier being interpreted as page table data
        pd[pdindex] = (unsigned long)vmm_get_physaddr((address_t)newptaddr) | 0x7; // user, r/w (can be overriden), present
        __invlpg((unsigned long)pd);
	}

    // the kernel page tables are in a specific part of memory
    if (virtualaddr >= 0xC0000000) { ptphys = page_table_physaddr + (PAGE_SIZE * pdindex); } 
    else { ptphys = pd[pdindex] & 0xFFFFF000; }

    if ((pt[ptindex] & 1) == 1) {
		// this is present
		if (!overwrite) {
			return -1; // we failed
		}
	}

    pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | 0x01; // Present

    __invlpg(virtualaddr);
	return 0;
}

// map pages in kernelspace
void *vmm_map_pages_k(address_t *pages, int pagecount) {
    void *startblock;
    startblock = (void*)end_mapped_kmemory;
    int i;
    for (i = 0; i < pagecount; i++) {
        if (vmm_map_page(pages[i], end_mapped_kmemory, 0x3, 0) == -1) {
            return NULL;
        }
        end_mapped_kmemory += PAGE_SIZE;
        if (end_mapped_kmemory >= 0xFFC00000) { // we overflowed into our page directory, out of virtual memory
            panic("Out of virtual memory");
        }
    }
    return startblock;
}

// map pages in userspace
void *vmm_map_pages_u(address_t *pages, int pagecount, struct process *ps) {
    void *startblock;
    startblock = (void*)ps->pgbrk;
    int i;
    for (i = 0; i < pagecount; i++) {
        if (vmm_map_page(pages[i], ps->pgbrk, 0x7, 0) == -1) {
            startblock = NULL;
        }
        ps->pgbrk += PAGE_SIZE;
        if (ps->pgbrk >= 0xC0000000) { // we overflowed into kernel space, out of virtual memory
            startblock = NULL;
        }
    }
    return startblock;
}

// map pages at specific location
// loc will be modified
void *vmm_map_pages_loc(address_t *pages, int pagecount, address_t loc, unsigned int flags) {
    void *startblock;
    startblock = (void*)loc;
    int i;
    for (i = 0; i < pagecount; i++) {
        if (vmm_map_page(pages[i], loc, flags, 0) == -1) {
            return NULL;
        }
        loc += PAGE_SIZE;
        if (loc >= 0xFFC00000) { // we overflowed into our page directory, out of virtual memory
            return NULL;
        }
    }
    return startblock;
}

void vmm_map_stack(struct thread *thread) {
    unsigned long *pd = (unsigned long *)processes[thread->pid]->cr3_virt;

    // find the highest possible directory entry for the stack to be located in
    int stackpdi = 767;
    while (pd[stackpdi] & 0x1) {
        stackpdi--;
        if (stackpdi == -1) {
            // h
            break;
        }
    }
    thread->stackpdi = stackpdi;
    if (stackpdi > -1) {
        void* newptaddr = liballoc_alloc(1); // get a page for the page table
        memset(newptaddr, 0, PAGE_SIZE); // zero it out, we don't want any random data that was in there earlier being interpreted as page table data
        pd[stackpdi] = (unsigned long)vmm_get_physaddr((address_t)newptaddr) | 0x7; // user, r/w (can be overriden), present
    } 
}

int vmm_unmap_page(address_t virtualaddr) {
    unsigned long pdindex = (unsigned long)virtualaddr >> 22;
    unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

    unsigned long *pd = (unsigned long *)PAGE_DIRECTORY_ADDR;
    unsigned long *pt = ((unsigned long *)PAGE_TABLE_ADDR) + (0x400 * pdindex);
	if ((pd[pdindex] & 1) == 0) {
		// this isn't present
		return 0;
	}

    if ((pt[ptindex] & 1) == 0) {
		// this isn't present
		return 0;
	}

    pt[ptindex] = 0x00000000;

    __invlpg(virtualaddr);
	return 1;
}

void vmm_init(unsigned int pagetable) {
    page_table_physaddr = pagetable;
    unsigned long *pd = (unsigned long *)PAGE_DIRECTORY_ADDR;
    for (int i = 3; i < 255; i++) {
        pd[i + 768] = page_table_physaddr + (PAGE_SIZE * i) + 0x3;
    }
}