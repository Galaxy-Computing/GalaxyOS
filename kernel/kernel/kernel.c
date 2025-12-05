#include <stdio.h>

// holy includes
#include <kernel/tty.h>
#include <kernel/vga.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/fpu.h>
#include <kernel/klog.h>
#include <kernel/ata.h>
#include <kernel/ps2.h>
#include <kernel/kernel.h>
#include <kernel/ps2kb.h>
#include <kernel/multiboot.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/liballoc.h>

const char *version = "GalaxyOS Neptune 0.1.0-dev+7";
int bootfinished = 0;

void kernel_main(multiboot_info_t* mbd, unsigned int magic, unsigned int pagetable) {
	multiboot_info_t* vmbd = (multiboot_info_t*)((char*)mbd + 0xC0000000); // convert it to a virtual address
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		return;
		// todo: panic with invalid magic
	}
	if(!(vmbd->flags >> 6 & 0x1)) {
        return;
		// todo: panic with no memory map
    }

	gdt_setup();
	idt_setup();
	irq_install();
	terminal_initialize();
	log_ok("Terminal initialized");
	if (init_fpu()) {
		log_fail("FPU not found. System cannot continue. Halting!");
		return;
	}
	log_ok("FPU initialized");
	pmm_init(vmbd);
	vmm_init(pagetable);
	ps2_init();
	ps2kb_init();
	atapio_detect_disks();
	terminal_setfgcolor(VGA_COLOR_LIGHT_GREY);
	printf("Welcome to %s\n",version);
	
	bootfinished = 1;

	uint32_t* testalloc = (uint32_t*)kmalloc(sizeof(uint32_t));
	if (testalloc == NULL) {
		// we have a problem
		printf("Test allocate failed\n");
	} else {
		*testalloc = 1;
		printf("Test allocate: %i\n", *testalloc);
		printf("Address: %x\n", testalloc);
	}
	kfree((void*)testalloc); // free it, we don't need it anymore

	while (1) { } // spin
}