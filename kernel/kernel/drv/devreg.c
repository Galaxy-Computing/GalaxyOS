// Device Register (devreg.c)
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

#include <kernel/devreg.h>
#include <kernel/liballoc.h>
#include <kernel/klog.h>
#include <kernel/devcfg.h>

#include <string.h>
#include <stdio.h>

#define DEVREG_DEFAULT_SIZE 128

struct device *device_register;
uint32_t device_register_size;
uint32_t last_device = 0;

struct device *devreg_find_device_by_kid(uint8_t kid) {
    for (uint32_t i = 0; i < last_device; i++) {
        if (device_register[i].k_id == kid) {
            return &device_register[i];
        }
    }
    return NULL; // no device exists for this KID
}

uint32_t devreg_register_device(struct device *newdevice) {
    if (devreg_find_device_by_kid(newdevice->k_id) != NULL) {
        #ifdef VGATEXT
        terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	    printf("[");
	    terminal_setfgcolor(VGA_COLOR_LIGHT_BROWN);
	    printf("WARN");
	    terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	    printf("] [DEVREG] Device KID conflict detected. The kernel device will still be registered, but it will not be used. Consider rebuilding the kernel without the conflicting device. \"%s\" (%i)\n", newdevice->name, newdevice->k_id);
        #endif
    }
    if (last_device >= device_register_size) {
        device_register = (struct device*)krealloc((void*)device_register, sizeof(struct device) * (device_register_size+DEVREG_DEFAULT_SIZE));
        device_register_size = device_register_size+DEVREG_DEFAULT_SIZE;
    }
    memcpy(&device_register[last_device], newdevice, sizeof(struct device));
    device_register[last_device].id = last_device;
    return last_device++;
}

struct device *devreg_get_device_info(uint32_t devid) {
    if (last_device - 1 > devid) {
        return NULL; // this device doesn't exist
    }
    return &device_register[devid];
}

void devreg_init(void) {
    device_register = (struct device*)kmalloc(sizeof(struct device) * DEVREG_DEFAULT_SIZE);
    device_register_size = DEVREG_DEFAULT_SIZE;
}
