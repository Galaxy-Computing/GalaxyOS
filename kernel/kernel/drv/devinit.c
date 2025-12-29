// Device Initializer (devinit.c)
// Copyright (C) 2025 Skye310 (Galaxy Computing)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <kernel/devcfg.h>
#include <kernel/devreg.h>
#include <kernel/liballoc.h>
#include <kernel/klog.h>

void devinit(void) {
    struct device temp;
    // Add any kernel device (that is not a terminal) that may need to be initialized here.
    #ifdef PS2
    temp.name = "I8042 PS/2 Controller";
    temp.privilege = 0;
    temp.type = DEVICE_HID;
    temp.k_id = PS2;
    devreg_register_device(&temp);
    ps2_init();
    #endif

    #ifdef PS2KB
    temp.name = "PS/2 Keyboard";
    temp.privilege = 0;
    temp.type = DEVICE_HID;
    temp.k_id = PS2KB;
    devreg_register_device(&temp);
	ps2kb_init();
    #endif

    #ifdef ATAPIO
    temp.name = "Basic ATAPIO Interface";
    temp.privilege = 0;
    temp.type = DEVICE_INTERNAL;
    temp.k_id = ATAPIO;
    devreg_register_device(&temp);
	atapio_detect_disks();
    #endif

    #ifdef VFS
    temp.name = "Virtual File System";
    temp.privilege = 0;
    temp.type = DEVICE_INTERNAL;
    temp.k_id = ATAPIO;
    devreg_register_device(&temp);
	vfs_init();
    #endif

    #ifdef RDISK
    temp.name = "RAM Disk Interface";
    temp.privilege = 0;
    temp.type = DEVICE_BLOCK;
    temp.k_id = ATAPIO;
    devreg_register_device(&temp);
    rdisk_init();
    #endif

    #ifdef VFAT
    temp.name = "VFAT FS Driver";
    temp.privilege = 0;
    temp.type = DEVICE_FILESYSTEM;
    temp.k_id = ATAPIO;
    devreg_register_device(&temp);
    rdisk_init();
    #endif

    log_ok("[DEVREG] Device initialization complete.");
}

void devinit_tty(void) {
    struct device temp;
    // Add any terminal device that may need to be initialized here.
    #ifdef VGATEXT
    temp.name = "VGA Text Terminal";
    temp.privilege = 0;
    temp.type = DEVICE_DISPLAY;
    temp.k_id = VGATEXT;
    devreg_register_device(&temp);
    terminal_initialize();
    #endif
}

void devinit_init(void) {
    devreg_init();
}
