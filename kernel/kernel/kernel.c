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
#include <kernel/exception.h>

int bootfinished = 0;

void kernel_main(multiboot_info_t* mbd, unsigned int magic, unsigned int pagetable) {
	terminal_initialize();
	log_ok("Terminal initialized");

	multiboot_info_t* vmbd = (multiboot_info_t*)((char*)mbd + 0xC0000000); // convert it to a virtual address
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		panic("The OS was not loaded with a multiboot compliant bootloader");
	}
	if(!(vmbd->flags >> 6 & 0x1)) {
		panic("The bootloader did not provide a memory map");
    }

	gdt_setup();
	idt_setup();
	irq_install();
	if (init_fpu()) {
		panic("CPU not supported");
		return;
	}
	log_ok("FPU initialized");
	pmm_init(vmbd);
	vmm_init(pagetable);
	ps2_init();
	ps2kb_init();
	atapio_detect_disks();
	terminal_setfgcolor(VGA_COLOR_LIGHT_GREY);
	printf("Welcome to %s\n",K_VERSION);
	
	bootfinished = 1;

	uint32_t* testalloc = (uint32_t*)kmalloc(sizeof(uint32_t));
	if (testalloc == NULL) {
		// we have a problem
		panic("Test allocate failed");
	} else {
		*testalloc = 1;
		printf("Test allocate: %i\n", *testalloc);
		printf("Address: 0x%x\n", testalloc);
	}
	kfree((void*)testalloc); // free it, we don't need it anymore

	while (1) { } // spin
}