#include <stdint.h>
#include <kernel/pmm.h>
#include <kernel/multiboot.h>
#include <stdio.h>
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <kernel/klog.h>

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
		//panic("Out of physical memory."); todo: actually implement this function
		halt();
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
	terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	printf("[");
	terminal_setfgcolor(VGA_COLOR_LIGHT_BLUE);
	printf("INFO");
	terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	printf("] [PMM] Detected %iM bytes of memory usable.\n", pmm_available() / 256);
}