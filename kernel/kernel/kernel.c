#include <stdio.h>

// holy includes
#include <kernel/devcfg.h>
#include <kernel/vga.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/fpu.h>
#include <kernel/klog.h>
#include <kernel/kernel.h>
#include <kernel/multiboot.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/liballoc.h>
#include <kernel/exception.h>
#include <kernel/pload.h>
#include <kernel/sched.h>

int bootfinished = 0;

void kernel_loop(void) {
	//sched_loop(); this isn't ready yet
}

void kernel_main(multiboot_info_t* mbd, unsigned int magic, unsigned int pagetable) {
	multiboot_info_t* vmbd = (multiboot_info_t*)((char*)mbd + 0xC0000000); // convert it to a virtual address

	gdt_setup();
	idt_setup();
	irq_install();
	pmm_init(vmbd);
	vmm_init(pagetable);

	devinit_init();
	devinit_tty();
	log_ok("Terminal initialized");

	pmm_log();
	
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		panic("The OS was not loaded with a multiboot compliant bootloader");
	}
	if(!(vmbd->flags >> 6 & 0x1)) {
		panic("The bootloader did not provide a memory map");
    }

	if (init_fpu()) {
		panic("CPU not supported");
	}
	log_ok("FPU initialized");
	
	devinit();
	
	sched_init();
	pload_create_process_k();

	terminal_setfgcolor(VGA_COLOR_LIGHT_GREY);
	printf("Welcome to %s\n",K_VERSION);
	
	/*uint32_t* testalloc = (uint32_t*)kmalloc(sizeof(uint32_t));
	if (testalloc == NULL) {
		// we have a problem
		panic("Test allocate failed");
	} else {
		*testalloc = 1;
		printf("Test allocate: %i\n", *testalloc);
		printf("Address: 0x%x\n", testalloc);
	}
	printf("Memory free: %ikb\n", pmm_available() * 4);
	kfree((void*)testalloc); // free it, we don't need it anymore*/

	// We would start our init process here if we had it

	bootfinished = 1;
	asm("sti");
	while (1) { kernel_loop(); } // spin
}

