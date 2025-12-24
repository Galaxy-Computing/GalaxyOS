// Virtual Memory Manager (vmm.c)
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


#include <stdint.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/exception.h>
#include <stddef.h>

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
    unsigned long ptphys = page_table_physaddr + (PAGE_SIZE * pdindex);
	if ((pd[pdindex] & 1) == 0) {
		// this isn't present, create a new one
		pd[pdindex] = ptphys + 0x7; // user (this can be overriden by the pages), r/w (also can be overriden), present
        __invlpg((unsigned long)pd);
	}

    if ((pt[ptindex] & 1) == 1) {
		// this is present
		if (!overwrite) {
			return 0; // we failed
		}
	}

    pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | 0x01; // Present

    __invlpg(virtualaddr);
	return 1;
}

void *vmm_map_pages_k(address_t *pages, int pagecount) {
    void *startblock;
    startblock = (void*)end_mapped_kmemory;
    int i;
    for (i = 0; i < pagecount; i++) {
        if (!vmm_map_page(pages[i], end_mapped_kmemory, 0x3, 0)) {
            return NULL;
        }
        end_mapped_kmemory += PAGE_SIZE;
        if (end_mapped_kmemory >= 0xFFC00000) { // we overflowed into our page directory, out of virtual memory
            panic("Out of virtual memory");
        }
    }
    return startblock;
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
}