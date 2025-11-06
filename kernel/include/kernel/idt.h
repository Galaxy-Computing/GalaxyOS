#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H

struct idt_ptr;
/* This defines what the stack looks like after an ISR was running */
struct regs
{
    unsigned int gs, fs, es, ds;      /* pushed the segs last */
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    unsigned int int_no, err_code;    /* our 'push byte #' and ecodes do this */
    unsigned int eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */ 
};

struct idt_entry;

void idt_setup();
void idt_set_gate(unsigned char num, void(*func)(), unsigned char flags);

#endif