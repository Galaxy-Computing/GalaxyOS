// Liballoc interface with kernel (liballoc-sys.c)
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

#include <internal/liballoc.h>
#include <stdlib.h>
#include <unistd.h>

#define PAGE_SIZE 0x1000

static int lock = 0;

int liballoc_lock(void) {
    if (!lock) { lock = 1; return 0; }
    return -1;
}

int liballoc_unlock(void) {
    if (lock) { lock = 0; return 0; }
    return -1;
}

void *liballoc_alloc(size_t pages) {
    void* retval = sbrk(pages*PAGE_SIZE);
    if ((int)retval == -1) { return NULL; }
    return (void*)retval;
}

int liballoc_free(void* addr, size_t pages) {
    // always return success, despite not freeing anything.
    // this will obviously cause memory leaks, but for now this is fine just to get a functional implementation.
    return 0;
}