// Kernel Terminal Abstraction Driver (term.c)
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

#include <stddef.h>
#include <kernel/term.h>
#include <kernel/devcfg.h>
#include <kernel/sched.h>

// New code should be added here when a new terminal or keyboard driver is added, for now we just use the VGATEXT one.

#if defined(VGATEXT) && defined(PS2KB)
struct terminal defaultterm = {
    .write = &terminal_write,
    .read  = &ps2kb_read_chars,
    .clear = &terminal_clear,
    .id    = 0
};
#else
struct terminal defaultterm = {
    .write = NULL,
    .read  = NULL,
    .clear = NULL,
    .id    = 0
};
#endif

struct terminal *currentterm = &defaultterm;

size_t term_write(const char *str, size_t size) {
    #ifdef VGATEXT
    return terminal_write(str, size);
    #endif
}

size_t term_read(char *buf, size_t size) {
    size_t bytes_read = 0;
    #ifdef PS2KB
    bytes_read += ps2kb_read_chars(buf, size);
    #endif
    //if (bytes_read >= size) { return bytes_read; } // this is only needed if we have more than one keyboard driver
    return bytes_read;
}

size_t term_readline(char *buf, size_t size) {
    #ifdef PS2KB
    ps2kb_clear_keybuf();
    #endif
    size_t bytes_read = 0;
    size_t old_bytes_read = 0;
    while (bytes_read < size) {
        sched_suspend_current_thread(1); // wait for a keyboard int
        bytes_read += term_read(&buf[bytes_read], 1);
        if (bytes_read > old_bytes_read) {
            term_write(&buf[bytes_read-1], 1);
            old_bytes_read = bytes_read;
            if (buf[bytes_read-1] == 0x7F) {
                buf[bytes_read-1] = 0;
                bytes_read--;
                old_bytes_read--;
                if (bytes_read) {
                    buf[bytes_read-1] = 0;
                    bytes_read--;
                    old_bytes_read--;
                }
            }
            if (buf[bytes_read-1] == '\n') {
                buf[bytes_read-1] = 0;
                bytes_read--;
                return bytes_read;
            }
        }
    }
    return bytes_read;
}

void term_clear(void) {
    #ifdef VGATEXT
    terminal_clear();
    return;
    #endif
}