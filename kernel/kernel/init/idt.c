#include <kernel/idt.h>
#include <kernel/exception.h>

#include <string.h>

/* Defines an IDT entry */
struct idt_entry {
    unsigned short offset_lo;
    unsigned short sel;        
    unsigned char always0;     
    unsigned char flags;       
    unsigned short offset_hi;
} __attribute__((packed));


struct idt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

/* Declare an IDT of 256 entries. */
struct idt_entry idt[256];
struct idt_ptr idtp;

/* This exists in 'start.asm', and is used to load our IDT */
extern void idt_load(void);

void idt_set_gate(unsigned char num, void(*func)(), unsigned char flags) {
    idt[num].offset_lo = (unsigned short)((unsigned int)func & 0xFFFF);
    idt[num].offset_hi = (unsigned short)(((unsigned int)func >> 16) & 0xFFFF);

    idt[num].sel = 0x8;
    idt[num].flags = flags;

    idt[num].always0 = 0;
}

void idt_setup(void) {
    /* Sets the special IDT pointer up, just like in 'gdt.c' */
    idtp.limit = (sizeof (struct idt_entry) * 256) - 1;
    idtp.base = (unsigned int)&idt;

    /* Clear out the entire IDT, initializing it to zeros */
    memset(&idt, 0, sizeof(struct idt_entry) * 256);

    isrs_install();

    /* Points the processor's internal register to the new IDT */
    idt_load();
}
		