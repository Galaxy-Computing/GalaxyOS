// System Call Handler (syscall.c)
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

#include <kernel/irq.h>
#include <kernel/vfs.h>
#include <kernel/pload.h>
#include <kernel/sched.h>
#include <kernel/kernel.h>
#include <sys/types.h>
#include <errno.h>

typedef int (*syscallfunc)(unsigned int[]);

syscallfunc syscalltable[256] = {0};

int syscall_exit(unsigned int args[]) {
    return sched_exit_process((int)args[0]);
}

int syscall_wait(unsigned int args[]) {
    return sched_wait_process((int)args[0]);
}

int syscall_read(unsigned int args[]) {
    switch ((int)args[0]) {
        case 0: // stdin
            return currentps->terminal->read((char*)args[1], (size_t)args[2]);
        case 1: // stdout
        case 2: // stderr
            return 0;
    }
    return vfs_read((int)args[0]-3, (void*)args[1], (size_t)args[2]);
}

int syscall_write(unsigned int args[]) {
    switch ((int)args[0]) {
        case 0: // stdin
            return 0;
        case 1: // stdout
        case 2: // stderr
            return currentps->terminal->write((const char*)args[1], (size_t)args[2]);
    }
    return vfs_write((int)args[0]-3, (const void*)args[1], (size_t)args[2]);
}

int syscall_open(unsigned int args[]) {
    return vfs_open((char*)args[0], (int)args[1], (mode_t)args[2])+3;
}

int syscall_exec(unsigned int args[]) {
    return pload_create_process_file((char*)args[0], (char**)args[1]);
}

int syscall_brk(unsigned int args[]) {
    return sched_setbrk((void*)args[0]);
}

int syscall_eret(unsigned int args[]) {
    if ((int)args[0] < 0) {
        // the process has requested to exit
        return sched_exit_process(-(int)args[0]);
    }
    // we don't need to do anything if the process has returned any other value
}

int syscall_ehndlr(unsigned int args[]) {
    if ((int)args[0] < 0) {
        // the process has requested to exit
        return sched_exit_process(-(int)args[0]);
    }
    // we don't need to do anything if the process has returned any other value
}

void syscall_handler(struct regs *r) {
    //asm("sti"); // allow syscalls to be preempted (may cause weirdness but i hope it works)
    kmode = 0;
    if (syscalltable[r->eax]) {
        unsigned int args[] = {r->ecx, r->edx, r->ebx, r->esi, r->edi, r->ebp};
        r->eax = syscalltable[r->eax](args);
    } else {
        r->eax = -ENOSYS;
    }
    kmode = 1;
    //asm("cli");
}

void syscall_init(void) {
    irq_install_handler(0x60, &syscall_handler);

    syscalltable[1]  = &syscall_exit;
    syscalltable[2]  = &syscall_wait;
    syscalltable[3]  = &syscall_read;
    syscalltable[4]  = &syscall_write;
    syscalltable[5]  = &syscall_open;
    syscalltable[7]  = &syscall_exec;
    syscalltable[8]  = &syscall_brk;
    syscalltable[10] = &syscall_eret;
    syscalltable[11] = &syscall_ehndlr;
}

