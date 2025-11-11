#include <stdio.h>

#include <kernel/vga.h>
#include <kernel/tty.h>

void log_ok(const char* message) {
	terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	printf("[ ");
	terminal_setfgcolor(VGA_COLOR_GREEN);
	printf("OK ");
	terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	printf("] ");
	printf(message);
	printf("\n");
	terminal_setfgcolor(VGA_COLOR_LIGHT_GREY);
}

void log_info(const char* message) {
	terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	printf("[");
	terminal_setfgcolor(VGA_COLOR_LIGHT_BLUE);
	printf("INFO");
	terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	printf("] ");
	printf(message);
	printf("\n");
	terminal_setfgcolor(VGA_COLOR_LIGHT_GREY);
}

void log_fail(const char* message) {
	terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	printf("[");
	terminal_setfgcolor(VGA_COLOR_RED);
	printf("FAIL");
	terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	printf("] ");
	printf(message);
	printf("\n");
	terminal_setfgcolor(VGA_COLOR_LIGHT_GREY);
}