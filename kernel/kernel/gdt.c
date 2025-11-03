#include <kernel/gdt.h>

/* Defines a GDT entry. We say packed, because it prevents the
*  compiler from doing things that it thinks is best: Prevent
*  compiler "optimization" by packing */
struct gdt_entry
{
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));

/* Special pointer which includes the limit: The max bytes
*  taken up by the GDT, minus 1. Again, this NEEDS to be packed */
struct gdt_ptr
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

/* Our GDT, with 5 entries, and finally our special GDT pointer */
struct gdt_entry gdt[5];
struct gdt_ptr gp;

/* Setup a descriptor in the Global Descriptor Table */
void gdt_set_gate(int num, unsigned char access)
{
    /* Setup the descriptor base address */
    gdt[num].base_low = 0;
    gdt[num].base_middle = 0;
    gdt[num].base_high = 0;

    /* Setup the descriptor limits */
    gdt[num].limit_low = 0xFFFF;
    gdt[num].granularity = 0xCF;

    gdt[num].access = access;
}
void gdt_set_gate_null(int num)
{
    /* Setup the descriptor base address */
    gdt[num].base_low = 0;
    gdt[num].base_middle = 0;
    gdt[num].base_high = 0;

    /* Setup the descriptor limits */
    gdt[num].limit_low = 0;
    gdt[num].granularity = 0;

    gdt[num].access = 0;
}

void gdt_setup()
{
    /* Setup the GDT pointer and limit */
    gp.limit = (sizeof(struct gdt_entry) * 5) - 1;
    gp.base = (unsigned int)&gdt;

    /* Our NULL descriptor */
    gdt_set_gate_null(0);

    gdt_set_gate(1, 0x9A);
    gdt_set_gate(2, 0x92);
    gdt_set_gate(3, 0xFA);
    gdt_set_gate(4, 0xF2);

    load_gdt(&gp, 2 * 0x08, 1 * 0x08);
}