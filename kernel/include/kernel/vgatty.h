#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include <kernel/vga.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_setfgcolor(enum vga_color fg);
void terminal_setbgcolor(enum vga_color bg);
void terminal_clear(void);
void terminal_enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void terminal_update_cursor(int x, int y);
void terminal_disable_cursor(void);

#endif
