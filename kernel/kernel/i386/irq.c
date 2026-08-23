// IRQ Initalizer/Routines (irq.c)
// Copyright (C) 2025-2026 Skye310 (Galaxy Computing)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <kernel/irq.h>
#include <kernel/idt.h>
#include <kernel/io.h>
#include <kernel/sched.h>
#include <kernel/exception.h>
#include <stdio.h>
#include <stdint.h>

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
extern void irq16();
extern void irq0x80();

void *irq_routines[224] = {0};

void irq_install_handler(int irq, void (*handler)(struct regs *r)) {
    irq_routines[irq] = handler;
}

void irq_uninstall_handler(int irq) {
    irq_routines[irq] = 0;
}

void irq_remap(void) {
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();
    outb(0x21, 0x20);
    io_wait();
    outb(0xA1, 0x28);
    io_wait();
    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();
    outb(0x21, 0x0);
    io_wait();
    outb(0xA1, 0x0);
    io_wait();
}

void irq_install(void) {
    irq_remap();

    idt_set_gate(32, &irq0, 0x8E);
    idt_set_gate(33, &irq1, 0x8E);
    idt_set_gate(34, &irq2, 0x8E);
    idt_set_gate(35, &irq3, 0x8E);
    idt_set_gate(36, &irq4, 0x8E);
    idt_set_gate(37, &irq5, 0x8E);
    idt_set_gate(38, &irq6, 0x8E);
    idt_set_gate(39, &irq7, 0x8E);
    idt_set_gate(40, &irq8, 0x8E);
    idt_set_gate(41, &irq9, 0x8E);
    idt_set_gate(42, &irq10, 0x8E);
    idt_set_gate(43, &irq11, 0x8E);
    idt_set_gate(44, &irq12, 0x8E);
    idt_set_gate(45, &irq13, 0x8E);
    idt_set_gate(46, &irq14, 0x8E);
    idt_set_gate(47, &irq15, 0x8E);
    idt_set_gate(48, &irq16, 0xEE);
    idt_set_gate(0x80, &irq0x80, 0xEE);
    
    //asm("sti");
}

void irq_handler(struct regs *r) {
    if (currentps->procerr) {
        if (r->cs & 0x3) {
            if (currentps->procerrhandler != NULL) {
                // fake a call to procerrhandler
                // it's the thread's job to make sure the processor state does not get clobbered
                r->useresp -= 8;
                ((uint32_t*)r->useresp)[0] = r->eip;
                ((uint32_t*)r->useresp)[1] = currentps->procerr;
                r->eip = (uint32_t)&currentps->procerrhandler;
                currentps->procerr = 0;
            } else {
                // the process hasn't set one, so just kill it
                sched_exit_process(currentps->procerr);
                currentps->procerr = 0;
            }
            
        } else {
            char msgbuf[48];
            sprintf(msgbuf, "Kernel process crashed: code %i", currentps->procerr);
            panic(msgbuf);
        }
    }
    
    if (r->int_no > 32) {
        sched_check_suspended_threads(r->int_no - 32);
    }

    /* This is a blank function pointer */
    void (*handler)(struct regs *r);

    /* Find out if we have a custom handler to run for this
    *  IRQ, and then finally, run it */
    handler = irq_routines[r->int_no - 32];
    if (handler)
    {
        handler(r);
    }

    if (r->int_no >= 48) {
        return; // Since the interrupt didn't come from the interrupt controller we don't need to do anything else
    }
    /* If the IDT entry that was invoked was greater than 40
    *  (meaning IRQ8 - 15), then we need to send an EOI to
    *  the slave controller */
    if (r->int_no >= 40)
    {
        outb(0xA0, 0x20);
    }

    /* In either case, we need to send an EOI to the master
    *  interrupt controller too */
    outb(0x20, 0x20);
}

