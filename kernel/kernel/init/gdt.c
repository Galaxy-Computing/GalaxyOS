// Global Descriptor Table Initalizer (gdt.c)
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

#include <kernel/gdt.h>
#include <stdint.h>
#include <string.h>

struct gdt_entry_bits {
	unsigned int limit_low              : 16;
	unsigned int base_low               : 24;
	unsigned int accessed               :  1;
	unsigned int read_write             :  1; // readable for code, writable for data
	unsigned int conforming_expand_down :  1; // conforming for code, expand down for data
	unsigned int code                   :  1; // 1 for code, 0 for data
	unsigned int code_data_segment      :  1; // should be 1 for everything but TSS and LDT
	unsigned int DPL                    :  2; // privilege level
	unsigned int present                :  1;
	unsigned int limit_high             :  4;
	unsigned int available              :  1; // only used in software; has no effect on hardware
	unsigned int long_mode              :  1;
	unsigned int big                    :  1; // 32-bit opcodes for code, uint32_t stack for data
	unsigned int gran                   :  1; // 1 to use 4k page addressing, 0 for byte addressing
	unsigned int base_high              :  8;
} __attribute__((packed));

/* Special pointer which includes the limit: The max bytes
*  taken up by the GDT, minus 1. Again, this NEEDS to be packed */
struct gdt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct tss_entry_struct {
	uint32_t prev_tss; // The previous TSS - with hardware task switching these form a kind of backward linked list.
	uint32_t esp0;     // The stack pointer to load when changing to kernel mode.
	uint32_t ss0;      // The stack segment to load when changing to kernel mode.
	// Everything below here is unused.
	uint32_t esp1; // esp and ss 1 and 2 would be used when switching to rings 1 or 2.
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap_base;
} __attribute__((packed));

typedef struct tss_entry_struct tss_entry_t;

tss_entry_t tss_entry;

unsigned long get_esp_value(void) {
    unsigned long esp_value;
    // "movl %%esp, %0" moves the value of the ESP register into the output operand %0
    // "=r" indicates an output operand that will be stored in a general-purpose register
    // "esp" indicates that the ESP register is used as input
    __asm__ volatile("movl %%esp, %0" : "=r"(esp_value));
    return esp_value;
}

void write_tss(struct gdt_entry_bits *g) {
	// Compute the base and limit of the TSS for use in the GDT entry.
	uint32_t base = (uint32_t) &tss_entry;
	uint32_t limit = sizeof tss_entry;

	// Add a TSS descriptor to the GDT.
	g->limit_low = limit;
	g->base_low = base;
	g->accessed = 1; // With a system entry (`code_data_segment` = 0), 1 indicates TSS and 0 indicates LDT
	g->read_write = 0; // For a TSS, indicates busy (1) or not busy (0).
	g->conforming_expand_down = 0; // always 0 for TSS
	g->code = 1; // For a TSS, 1 indicates 32-bit (1) or 16-bit (0).
	g->code_data_segment=0; // indicates TSS/LDT (see also `accessed`)
	g->DPL = 0; // ring 0, see the comments below
	g->present = 1;
	g->limit_high = (limit & (0xf << 16)) >> 16; // isolate top nibble
	g->available = 0; // 0 for a TSS
	g->long_mode = 0;
	g->big = 0; // should leave zero according to manuals.
	g->gran = 0; // limit is in bytes, not pages
	g->base_high = (base & (0xff << 24)) >> 24; //isolate top byte

	// Ensure the TSS is initially zero'd.
	memset(&tss_entry, 0, sizeof tss_entry);

	tss_entry.ss0  = 0x10;  // Set the kernel stack segment.
	tss_entry.esp0 = get_esp_value(); // Set the kernel stack pointer.
	//note that CS is loaded from the IDT entry and should be the regular kernel code segment
}

/* Our GDT, with 6 entries, and finally our special GDT pointer */
struct gdt_entry_bits gdt[6];
struct gdt_ptr gp;

/* Setup a descriptor in the Global Descriptor Table */
void gdt_set_gate(int num, unsigned char access) {
    /* Setup the descriptor base address */
    gdt[num].base_low = 0;
    gdt[num].base_high = 0;

    /* Setup the descriptor limits */
    gdt[num].limit_low = 0xFFFF;
    gdt[num].limit_high = 0xF;

    gdt[num].gran = 1;
    gdt[num].big = 1;
    gdt[num].long_mode = 0;
    gdt[num].available = 0;

    gdt[num].present = access >> 7;
    gdt[num].DPL = (access >> 5) & 0b11;
    gdt[num].code_data_segment = (access >> 4) & 1;
    gdt[num].code = (access >> 3) & 1;
    gdt[num].conforming_expand_down = (access >> 2) & 1;
    gdt[num].read_write = (access >> 1) & 1;
    gdt[num].accessed = access & 1;
}

void gdt_set_gate_null(int num) {
    /* Setup the descriptor base address */
    gdt[num].base_low = 0;
    gdt[num].base_high = 0;

    /* Setup the descriptor limits */
    gdt[num].limit_low = 0;
    gdt[num].limit_high = 0;

    gdt[num].gran = 0;
    gdt[num].big = 0;
    gdt[num].long_mode = 0;
    gdt[num].available = 0;

    gdt[num].present = 0;
    gdt[num].DPL = 0;
    gdt[num].code_data_segment = 0;
    gdt[num].code = 0;
    gdt[num].conforming_expand_down = 0;
    gdt[num].read_write = 0;
    gdt[num].accessed = 0;
}

void gdt_setup(void) {
    /* Setup the GDT pointer and limit */
    gp.limit = (sizeof(struct gdt_entry_bits) * 5) - 1;
    gp.base = (unsigned int)&gdt;

    /* Our NULL descriptor */
    gdt_set_gate_null(0);

    gdt_set_gate(1, 0x9A);
    gdt_set_gate(2, 0x92);
    gdt_set_gate(3, 0xFA);
    gdt_set_gate(4, 0xF2);

    write_tss(&gdt[5]);

    load_gdt(&gp);
}

void set_kernel_stack(uint32_t stack) { // Used when an interrupt occurs
	tss_entry.esp0 = stack;
}