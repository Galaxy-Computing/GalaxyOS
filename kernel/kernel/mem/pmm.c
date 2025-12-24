// Physical Memory Manager (pmm.c)
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
#include <kernel/multiboot.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <kernel/klog.h>
#include <kernel/exception.h>

#define PAGE_SIZE 0x1000

extern void halt(void); // fuk
extern unsigned long _kernel_end;
uint32_t kernel_end = (uint32_t)&_kernel_end;

static volatile address_t pmem_stack[MAX_PAGES];
static volatile address_t pmem_stack_top;

/* return number of free pages */
address_t pmm_available(void) {
	return pmem_stack_top;
}

/* allocate a single page */
address_t pmm_alloc_page(void) {
	// this code will be needed when we have paging to disk. right now, this is irrelevant since we don't have that ability.
	/*if(!pmem_stack_top) {
		//last effort to try to swap out something
		//vmm_need_pages();
	}*/

	if(!pmem_stack_top) {
		/* seems we couldn't free any */
		panic("Out of physical memory");
  	}

  	return pmem_stack[--pmem_stack_top];
}

/* free a single page */
void pmm_free_page(address_t paddr) {
	pmem_stack[pmem_stack_top++] = paddr;
}

void pmm_init(multiboot_info_t* mbd) {
    unsigned int i;
    for(i = 0; i < mbd->mmap_length; 
        i += sizeof(multiboot_memory_map_t)) 
    {
        multiboot_memory_map_t* mmmt = 
            (multiboot_memory_map_t*) (mbd->mmap_addr + i + 0xC0000000);

        /*printf("Start Addr: %x | Length: %x | Size: %x | Type: %d\n",
            mmmt->addr, mmmt->len, mmmt->size, mmmt->type);*/

        if(mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
            // we have a memory block
			unsigned int j;
			for (j = 0; j < mmmt->len_low; j += PAGE_SIZE) {
				if ((mmmt->addr_low + j) > (kernel_end - 0xC0000000)) {
					pmem_stack[pmem_stack_top++] = mmmt->addr_low + j;
				}
			}
        }
    }
}

void pmm_log(void) {
	terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	printf("[");
	terminal_setfgcolor(VGA_COLOR_LIGHT_BLUE);
	printf("INFO");
	terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	printf("] [PMM] Detected %iM bytes of memory usable.\n", pmm_available() / 256);
}