// Device Initializer (devinit.c)
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

#include <kernel/devcfg.h>
#include <kernel/liballoc.h>
#include <kernel/klog.h>

void devinit(void) {
    // Add any kernel device (that is not a terminal) that may need to be initialized here.
    #ifdef PS2
    ps2_init();
    #endif

    #ifdef PS2KB
	ps2kb_init();
    #endif

    #ifdef ATAPIO
	atapio_detect_disks();
    #endif

    #ifdef VFS
	vfs_init();
    #endif

    #ifdef RDISK
    rdisk_init();
    #endif

    #ifdef VFAT
    vfat_init();
    #endif

    log_ok("Device initialization complete.");
}

void devinit_tty(void) {
    // Add any terminal device that may need to be initialized here.
    #ifdef VGATEXT
    terminal_initialize();
    #endif
}