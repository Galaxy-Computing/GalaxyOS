// PS2 Keyboard Driver (ps2kb.c)
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

#include <stdio.h>

// holy includes
#include <kernel/devcfg.h>
#include <kernel/vga.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/fpu.h>
#include <kernel/klog.h>
#include <kernel/kernel.h>
#include <kernel/multiboot.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/liballoc.h>
#include <kernel/exception.h>
#include <kernel/pload.h>
#include <kernel/sched.h>
#include <kernel/term.h>

int bootfinished = 0;

void kernel_loop(void) {
    // We're in the kernel thread
    for (;;) {
        #ifdef PS2KB
        ps2kb_loop();
        char printingchar;
        size_t chars_read = term_read(&printingchar, 1);
        if (chars_read) {
            term_write(&printingchar, 1);
        }
        #endif
    }
}

void kernel_main(multiboot_info_t* mbd, unsigned int magic, unsigned int pagetable) {
    multiboot_info_t* vmbd = (multiboot_info_t*)((char*)mbd + 0xC0000000); // convert it to a virtual address

    gdt_setup();
    idt_setup();
    irq_install();
    pmm_init(vmbd);
    vmm_init(pagetable);

    devinit_init();
    devinit_tty();
    log_ok("Terminal initialized");

    pmm_log();
    
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("The OS was not loaded with a multiboot compliant bootloader");
    }
    if(!(vmbd->flags >> 6 & 0x1)) {
        panic("The bootloader did not provide a memory map");
    }

    if (init_fpu()) {
        panic("CPU not supported");
    }
    log_ok("FPU initialized");
    
    devinit();
    
    sched_init();
    sched_create_thread(pload_create_process_k((uint32_t*)vmm_get_physaddr(0xFFFFF000)), (uint32_t)&kernel_loop);
    // this is sort of a nasty hack
    sched_pick_next();
    sched_pick_next();

    terminal_setfgcolor(VGA_COLOR_LIGHT_GREY);
    printf("Welcome to %s\n",K_VERSION);

    // We would start our init process here if we had it

    bootfinished = 1;
    asm("sti");
    /* 
     * since the scheduler thinks that we are the kernel thread now due to the above pick_next calls, 
     * we need to hlt to wait for the proper context switch to set the stack and go to our entry point
     * this kind of sucks but we can't do much about this
     */
    asm("hlt");
    kernel_loop();
}

