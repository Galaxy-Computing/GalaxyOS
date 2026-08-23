// Exception Handler (exception.c)
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

#include <kernel/exception.h>
#include <kernel/vga.h>
#include <kernel/vgatty.h>
#include <kernel/kernel.h>
#include <stdio.h>

__attribute__((__noreturn__))
void panic(const char *message) {
    terminal_setbgcolor(VGA_COLOR_RED);
    terminal_setfgcolor(VGA_COLOR_WHITE);
    terminal_clear();
    printf("KERNEL PANIC\n");
    printf("Version: %s\n", K_VERSION);
    printf(message);
    asm(
        "cli\n\t"
        "hlt\n\t"
    );
    __builtin_unreachable();
}
