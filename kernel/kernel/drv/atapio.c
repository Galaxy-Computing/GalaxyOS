// ATAPIO Driver (atapio.c)
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

#include <kernel/ata.h>
#include <kernel/io.h>
#include <kernel/klog.h>
#include <kernel/tty.h>
#include <stdio.h>

// this driver should not be used after boot

unsigned short ata_databuf[256];
struct ata_device atadevices[4];

void atapio_identify(unsigned short bus, char select, struct ata_device *device) {
    device->bus = bus;
    device->select = select;

    outb(bus + 6, select);
    outb(bus + 2, 0);
    outb(bus + 3, 0);
    outb(bus + 4, 0);
    outb(bus + 5, 0);
    outb(bus + 7, 0xEC); // IDENTIFY

    // this is dumb but ATAPIO is dumber
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);

    if (inb(bus + 7) == 0) {
        // no device here
        device->type = 0;
    } else {
        while ((inb(bus + 7) & 0x80) == 0x80) {}
        unsigned lbamid = inb(bus + 4);
        unsigned lbahi = inb(bus + 5);
        if ((lbamid == 0x14) && (lbahi == 0xEB)) {
            device->type = 2; // ATAPI
            terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	        printf("[");
	        terminal_setfgcolor(VGA_COLOR_LIGHT_BLUE);
	        printf("INFO");
	        terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	        printf("] [ATAPIO] Detected ATAPI device in 0x%x:0x%x\n",bus,select);
            return;
        }
        if ((lbamid == 0x3c) && (lbahi == 0xc3)) {
            device->type = 3; // SATA
            terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	        printf("[");
	        terminal_setfgcolor(VGA_COLOR_LIGHT_BLUE);
	        printf("INFO");
	        terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	        printf("] [ATAPIO] Detected SATA device in 0x%x:0x%x\n",bus,select);
            return;
        }
        if ((lbamid == 0x69) && (lbahi == 0x96)) {
            device->type = 4; // SATAPI
            terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	        printf("[");
	        terminal_setfgcolor(VGA_COLOR_LIGHT_BLUE);
	        printf("INFO");
	        terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	        printf("] [ATAPIO] Detected SATAPI device in 0x%x:0x%x\n",bus,select);
            return;
        }
        if ((lbamid == 0) && (lbahi == 0)) {
            device->type = 1; // ATA
            terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	        printf("[");
	        terminal_setfgcolor(VGA_COLOR_LIGHT_BLUE);
	        printf("INFO");
	        terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
	        printf("] [ATAPIO] Detected ATA device in %x:%x\n",bus,(int)select);
            unsigned status = inb(bus + 7);
            while ((status & 9) == 0) { status = inb(bus + 7); }
            if ((status & 1) == 0) {
                for (int i = 0; i < 256; i++) { 
                    device->identifydata[i] = inb(bus);
                }
                device->identified = 1;
            }
        }
    }
}

void atapio_detect_disks(void) {
    if (inb(0x1F7) == 0xFF) { // primary
        // no disks here
    } else {
        atapio_identify(0x1F0, 0xA0, &atadevices[0]);
        atapio_identify(0x1F0, 0xB0, &atadevices[1]);
    }
    if (inb(0x177) == 0xFF) { // secondary
        // no disks here
    } else {
        atapio_identify(0x170, 0xA0, &atadevices[2]);
        atapio_identify(0x170, 0xB0, &atadevices[3]);
    }
}

char atapio_readstatus(unsigned short bus, char select) {
    outb(bus + 6, select);

    // this horribleness is to allow time for the bus to switch
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);
    inb(bus + 7);

    return inb(bus + 7);
}