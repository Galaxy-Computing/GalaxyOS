// Interrupt Descriptor Table Initalizer (idt.c)
// Copyright (C) 2025 Skye310 (Galaxy Computing)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
		