// x86 Exception Handler (exceptionx86.c)
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

#include <kernel/exception.h>
#include <kernel/idt.h>
#include <kernel/vga.h>
#include <kernel/vgatty.h>
#include <kernel/kernel.h>
#include <kernel/sched.h>
#include <stdio.h>
#include <stdarg.h>

extern void isr_0(void);
extern void isr_1(void);
extern void isr_2(void);
extern void isr_3(void);
extern void isr_4(void);
extern void isr_5(void);
extern void isr_6(void);
extern void isr_7(void);
extern void isr_8(void);
extern void isr_9(void);
extern void isr_10(void);
extern void isr_11(void);
extern void isr_12(void);
extern void isr_13(void);
extern void isr_14(void);
extern void isr_16(void);
extern void isr_17(void);
extern void isr_18(void);
extern void isr_19(void);
extern void isr_20(void);
extern void isr_21(void);
extern void isr_28(void);
extern void isr_29(void);
extern void isr_30(void);

char exbuf[96];

char cpumessages[][32] = {
    "Division Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Unknown"
};

void isrs_install(void) {
    idt_set_gate(0, &isr_0, 0x8F);
    idt_set_gate(1, &isr_1, 0x8F);
    idt_set_gate(2, &isr_2, 0x8F);
    idt_set_gate(3, &isr_3, 0x8F);
    idt_set_gate(4, &isr_4, 0x8F);
    idt_set_gate(5, &isr_5, 0x8F);
    idt_set_gate(6, &isr_6, 0x8F);
    idt_set_gate(7, &isr_7, 0x8F);
    idt_set_gate(8, &isr_8, 0x8F);
    idt_set_gate(9, &isr_9, 0x8F);
    idt_set_gate(10, &isr_10, 0x8F);
    idt_set_gate(11, &isr_11, 0x8F);
    idt_set_gate(12, &isr_12, 0x8F);
    idt_set_gate(13, &isr_13, 0x8F);
    idt_set_gate(14, &isr_14, 0x8F);
    idt_set_gate(16, &isr_16, 0x8F);
    idt_set_gate(17, &isr_17, 0x8F);
    idt_set_gate(18, &isr_18, 0x8F);
    idt_set_gate(19, &isr_19, 0x8F);
    idt_set_gate(20, &isr_20, 0x8F);
    idt_set_gate(21, &isr_21, 0x8F);
    idt_set_gate(28, &isr_28, 0x8F);
    idt_set_gate(29, &isr_29, 0x8F);
    idt_set_gate(30, &isr_30, 0x8F);
}

void exception_handle(struct regs *r) {
    if (currenttid != -1) {
        if (threads[currenttid]->privilege_level) {
            // hand the exception to the scheduler instead
            sched_user_fault(r->int_no, r->err_code);
            return;
        }
    }
    if (r->int_no < 32) {
        sprintf(exbuf, "CPU exception occurred: %s (0x%x)", cpumessages[r->int_no], r->int_no);
        panic(exbuf);
    }
}