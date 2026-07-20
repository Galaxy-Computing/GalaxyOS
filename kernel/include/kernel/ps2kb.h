#ifndef _KERNEL_PS2KB_H
#define _KERNEL_PS2KB_H

void ps2kb_init(void);
void ps2kb_loop(void);
size_t ps2kb_read_chars(char *buf, size_t size);
void ps2kb_clear_keybuf(void);

#endif