#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/fpu.h>
#include <kernel/klog.h>
#include <kernel/storage/ata.h>

void kernel_main(void) {
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
	atapio_detect_disks();
	printf("Welcome to GalaxyOS Neptune 0.1.0-dev+5\n");
	
	terminal_putchar(24/0); // this will crash on purpose
}