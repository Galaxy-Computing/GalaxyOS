#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>

void kernel_main(void) {
	gdt_setup();
	terminal_initialize();
	printf("GalaxyOS Neptune 0.1.0-alpha+2\n");
}
