#ifndef _KERNEL_IRQ_H
#define _KERNEL_IRQ_H

#include <kernel/idt.h>

void irq_install_handler(int irq, void (*handler)(struct regs *r));
void irq_install(void);

#endif