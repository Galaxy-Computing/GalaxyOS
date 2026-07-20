#ifndef _KERNEL_TERM_H
#define _KERNEL_TERM_H

#include <stddef.h>

void term_write(const char* str, size_t size);
size_t term_read(char *buf, size_t size);
size_t term_readline(char *buf, size_t size);

#endif