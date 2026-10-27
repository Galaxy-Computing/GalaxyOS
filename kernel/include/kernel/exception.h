#ifndef _KERNEL_EXCEPTION_H
#define _KERNEL_EXCEPTION_H

#include <kernel/idt.h>

void isrs_install(void);

void panic(const char *message);
extern void halt(void);

void exception_handle(struct regs *r);

#endif