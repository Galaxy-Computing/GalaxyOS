#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/fpu.h>
#include <kernel/klog.h>
#include <kernel/ata.h>
#include <kernel/ps2.h>
#include <kernel/kernel.h>
#include <kernel/ps2kb.h>

const char *version = "GalaxyOS Neptune 0.1.0-dev+6";
int bootfinished = 0;

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
	ps2_init();
	ps2kb_init();
	atapio_detect_disks();
	printf("Welcome to %s\n",version);
	bootfinished = 1;
	
	while (1) { } // spin
	//terminal_putchar(24/0); // this will crash on purpose
}