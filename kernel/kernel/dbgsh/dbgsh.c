// Kernel Debug Shell (dbgsh.c)
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

#include <kernel/term.h>
#include <kernel/kernel.h>
#include <kernel/sched.h>
#include <kernel/pmm.h>
#include <string.h>
#include <stdio.h>

char cmdbuf[256];

char* dbgsh_cmdline(void) {
    printf("kdbgsh> ");
    term_readline(cmdbuf, 256);
    return cmdbuf;
}

void dbgsh_main(void) {
    printf("You are now in kdbgsh.\n");
    printf("Type \"help\" for a list of commands.\n");
    int running = 1;
    while (running) {
        char* cmd = dbgsh_cmdline();
        if (!strcmp(cmd, "exit")) {
            running = 0;
        } 
        else if (!strcmp(cmd, "ver")) {
            printf("%s\n", K_VERSION);
        } 
        else if (!strcmp(cmd, "help")) {
            printf("exit - exit kdbgsh\n");
            printf("ver  - display kernel version\n");
            printf("ps   - list processes\n");
            printf("mem  - list memory usage (mb)\n");
            printf("memk - list memory usage (kb)\n");
        }
        else if (!strcmp(cmd, "ps")) {
            printf("id pl name\n");
            for (int i = 0; i < last_pid-1; i++) {
                printf("%i %i %s\n", i, processes[i]->privilege_level, processes[i]->name);
            }
        }
        else if (!strcmp(cmd, "memk")) {
            printf("used: %ikb\n", pmm_used() * 4);
            printf("allocated: %ikb\n", pmm_used_alloc() * 4);
            printf("available: %ikb\n", pmm_available() * 4);
        }
        else if (!strcmp(cmd, "mem")) {
            printf("used: %imb\n", pmm_used() / 256);
            printf("allocated: %imb\n", pmm_used_alloc() / 256);
            printf("available: %imb\n", pmm_available() / 256);
        }
    }
}