#include <kernel/exception.h>
#include <kernel/idt.h>
#include <stdio.h>

extern void isr_0();
extern void isr_1();
extern void isr_2();
extern void isr_3();
extern void isr_4();
extern void isr_5();
extern void isr_6();
extern void isr_7();
extern void isr_8();
extern void isr_9();
extern void isr_10();
extern void isr_11();
extern void isr_12();
extern void isr_13();
extern void isr_14();
extern void isr_16();
extern void isr_17();
extern void isr_18();
extern void isr_19();
extern void isr_20();
extern void isr_21();
extern void isr_28();
extern void isr_29();
extern void isr_30();

void isrs_install() {
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
    if (r->int_no < 32) {
        printf("Halt! Exception occurred\n");
        halt();
    }
}