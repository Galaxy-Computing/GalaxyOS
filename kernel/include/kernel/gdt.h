#ifndef _KERNEL_GDT_H
#define _KERNEL_GDT_H

struct gdt_ptr;
struct gdt_entry;

extern void load_gdt(struct gdt_ptr *gdt_ptr, unsigned int data_sel, unsigned int code_sel);
void gdt_set_gate(int num, unsigned char access);
void gdt_set_gate_null(int num);
void gdt_setup();



#endif