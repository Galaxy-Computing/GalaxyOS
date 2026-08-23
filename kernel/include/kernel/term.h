#ifndef _KERNEL_TERM_H
#define _KERNEL_TERM_H

#include <stddef.h>

typedef size_t (*twrite)(const char* str, size_t size);
typedef size_t (*tread)(char *buf, size_t size);
typedef void (*tclear)(void);

struct terminal {
    twrite write;
    tread  read;
    tclear clear;
    int id;
};

extern struct terminal defaultterm;

size_t term_write(const char* str, size_t size);
size_t term_read(char *buf, size_t size);
size_t term_readline(char *buf, size_t size);
void term_clear(void);

#endif