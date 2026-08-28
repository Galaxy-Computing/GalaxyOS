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
#include <kernel/dbgsh.h>
#include <kernel/atapi.h>
#include <kernel/syscall.h>
#include <string.h>

int kmode = 0;

char cmdline[512] = {0};

void kernel_loop(void) {
    // We're in the kernel thread

    // Suspend the thread until the init process dies
    sched_wait_process(2);
    asm("int $0x30"); // yield because the above function doesn't do that

    // If it ever does die, panic
    panic("Init process died");
}

// detect system volume and mount it
// either provide this with the name of a block device, or one of these special indicators:
// cd = the system volume is an atapi block device
void kernel_mount_system_volume(char* info) {
    struct vfs_block_device *bd = NULL;
    if (!strlen(info)) {
        // we weren't provided any information
        panic("Error finding system volume: No information provided");
    }
    if (!strcmp(info, "cd")) {
        if (atapi_disccount == 1) {
            // found the cd
            bd = &vfs_blockdevices[atapi_disc-1];
        } else if (atapi_disccount > 1) {
            panic("Error finding system volume: Please remove all other discs and reboot");
        } else {
            panic("Error finding system volume: No CD in drive");
        }
    } else {
        // check if there's a block device with the name provided
        struct vfs_block_device *bd = vfs_find_block_device_by_name(info);
        if (bd == NULL) {
            panic("Error finding system volume: The block device does not exist");
        } 
    }

    struct vfs_fs_driver* fsdriver = vfs_detect_fs(bd);
    if (fsdriver == NULL) {
        panic("Error finding system volume: Filesystem not supported");
    }

    if (vfs_mount(bd, fsdriver, false, K_SYSVOLNAME) == NULL) {
        panic("Error finding system volume: vfs_mount() call returned NULL");
    }
    log_ok("System volume mounted at \"local\"");
}

uint32_t kernel_main(multiboot_info_t* mbd, unsigned int magic, unsigned int pagetable) {
    multiboot_info_t* vmbd = (multiboot_info_t*)((char*)mbd + 0xC0000000); // convert it to a virtual address
    if ((vmbd->flags >> 2 & 0x1)) {
        strncpy(cmdline, (char*)(vmbd->cmdline + 0xC0000000), 512); // copy the cmdline into a variable that we know won't get trampled on later
    }
    gdt_setup();
    idt_setup();
    irq_install();
    pmm_init(vmbd);
    vmm_init(pagetable);
    syscall_init();

    devinit_tty();
    log_ok("Terminal initialized");

    pmm_log();
    
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("The OS was not loaded with a multiboot compliant bootloader");
    }
    if (!(vmbd->flags >> 6 & 0x1)) {
        panic("The bootloader did not provide a memory map");
    }

    if (init_fpu()) {
        panic("CPU not supported");
    }
    log_ok("FPU initialized");
    
    devinit();

    // mount the system volume
    kernel_mount_system_volume(cmdline);
    
    sched_init();
    sched_create_thread(sched_create_process(0, "glxykrnl.elf"), 0, (uint32_t)&kernel_loop);
    // this is sort of a nasty hack
    sched_pick_next();
    sched_pick_next();

    // we pass init an empty string here to tell it that it is being executed from the kernel
    if (pload_create_process_file(K_SYSVOLNAME ":galaxyos/init.elf", (char *[]){"", NULL}, 1) < 0) {
        printf("creating init process returned negative!!\n");
    }
    //sched_create_thread(sched_create_process(0, "kdbgsh"), 0, (uint32_t)&dbgsh_main);
    kmode = 1;
    
    /* 
     * since the scheduler thinks that we are the kernel thread now due to the above pick_next calls, 
     * we need to hlt to wait for the proper context switch to set the stack and go to our entry point
     * this kind of sucks but we can't do much about this
     */
    asm(
        "sti\n\t"
        "hlt\n\t"
    );
    kernel_loop();
}

