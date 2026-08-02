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
#include <sys/types.h>
#include <errno.h>

typedef int (*syscallfunc)(unsigned int[]);

syscallfunc syscalltable[256] = {0};

int syscall_open(unsigned int args[]) {
    return vfs_open((char*)args[0], (int)args[1], (mode_t)args[2]);
}

void syscall_handler(struct regs *r) {
    if (syscalltable[r->eax]) {
        unsigned int args[] = {r->ecx, r->edx, r->ebx, r->esi, r->edi, r->ebp};
        r->eax = syscalltable[r->eax](args);
    } else {
        r->eax = -ENOSYS;
    }
}

void syscall_init(void) {
    irq_install_handler(0x60, &syscall_handler);

    syscalltable[5] = &syscall_open;
}

