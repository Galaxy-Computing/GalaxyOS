// ATAPI Driver (atapi.c)
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

#include <kernel/io.h>
#include <kernel/ata.h>
#include <kernel/atapi.h>
#include <kernel/sched.h>
#include <kernel/exception.h>
#include <kernel/vfs.h>
#include <kernel/vga.h>
#include <kernel/vgatty.h>
#include <kernel/liballoc.h>
#include <stdio.h>

#define CDROM_SECTOR_SIZE 2048

int read_cdrom(struct ata_device *device, uint32_t lba, uint32_t sectors, uint16_t *buffer) {
    if (device->type != ATA_TYPE_ATAPI) {
        panic("read_cdrom() called on non ATAPI device");
    }

    uint16_t port = device->bus;

    // The command
	volatile uint8_t read_cmd[12] = {0xA8, 0,
	                                (lba >> 0x18) & 0xFF, (lba >> 0x10) & 0xFF, (lba >> 0x08) & 0xFF,
	                                (lba >> 0x00) & 0xFF,
	                                (sectors >> 0x18) & 0xFF, (sectors >> 0x10) & 0xFF, (sectors >> 0x08) & 0xFF,
	                                (sectors >> 0x00) & 0xFF,
	                                0, 0};

	outb(port + DRIVE_SELECT, device->select); // Drive select
	ata_io_wait(port);
	outb(port + ERROR_R, 0x00); 
	outb(port + LBA_MID, 2048 & 0xFF);
	outb(port + LBA_HIGH, 2048 >> 8);
	outb(port + COMMAND_REGISTER, 0xA0); // Packet command
	ata_io_wait(port);
 
    // Poll until ready (since it doesn't send an IRQ after the packet command)
    while (1) {
        uint8_t status = inb(port + COMMAND_REGISTER);
        if (status & 0x01)
            return 1;
        if (!(status & 0x80) && (status & 0x08))
            break;
    }
	
    // Send command
	outsw(port + DATA, (uint16_t *) read_cmd, 6);

    // Read words
	for (uint32_t i = 0; i < sectors; i++) {
        // Suspend the thread until the drive is ready
        if (port == 0x1F0) {
            sched_suspend_current_thread(14);
        } else {
            sched_suspend_current_thread(15);
        }

        if (inb(port + COMMAND_REGISTER) & 1) {
            // error occurred, stop the transfer and return a failure
            return 0;
        }

		int size = inb(port + LBA_HIGH) << 8
		           | inb(port + LBA_MID); // Get the size of transfer

		insw(port + DATA, (uint16_t *) ((uint8_t *) buffer + i * 0x800), size / 2); // Read it
	}

	return 0;
}

uint32_t get_cdrom_capacity(struct ata_device *device) {
    if (device->type != ATA_TYPE_ATAPI) {
        panic("get_cdrom_capacity() called on non ATAPI device");
    }

    uint16_t port = device->bus;

    // The command
	volatile uint8_t cmd[12] = {0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	outb(port + DRIVE_SELECT, device->select); // Drive select
	ata_io_wait(port);
	outb(port + ERROR_R, 0x00); 
	outb(port + LBA_MID, 2048 & 0xFF);
	outb(port + LBA_HIGH, 2048 >> 8);
	outb(port + COMMAND_REGISTER, 0xA0); // Packet command
	ata_io_wait(port);
 
    // Poll until ready (since it doesn't send an IRQ after the packet command)
    while (1) {
        uint8_t status = inb(port + COMMAND_REGISTER);
        if (status & 0x01)
            return 1;
        if (!(status & 0x80) && (status & 0x08))
            break;
    }
	
    // Send command
	outsw(port + DATA, (uint16_t *) cmd, 6);

    // Suspend the thread until the drive is ready
    if (port == 0x1F0) {
        sched_suspend_current_thread(14);
    } else {
        sched_suspend_current_thread(15);
    }

    if (inb(port + COMMAND_REGISTER) & 1) {
        // error occurred, assume there's no disc here
        return 0;
    }

    int size = inb(port + LBA_HIGH) << 8
		           | inb(port + LBA_MID); // Get the size of transfer

    uint32_t buf[size / 4];

    // Read it
    insw(port + DATA, (uint16_t*)buf, size / 2); 

	return buf[0] + 1;
}

// this is only used during the boot process when scheduling isn't set up
uint32_t get_cdrom_capacity_polling(struct ata_device *device) {
    if (device->type != ATA_TYPE_ATAPI) {
        panic("get_cdrom_capacity() called on non ATAPI device");
    }

    uint16_t port = device->bus;

    // The command
	volatile uint8_t cmd[12] = {0x25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	outb(port + DRIVE_SELECT, device->select); // Drive select
	ata_io_wait(port);
	outb(port + ERROR_R, 0x00); 
	outb(port + LBA_MID, 2048 & 0xFF);
	outb(port + LBA_HIGH, 2048 >> 8);
	outb(port + COMMAND_REGISTER, 0xA0); // Packet command
	ata_io_wait(port);
 
    // Poll until ready
    while (1) {
        uint8_t status = inb(port + COMMAND_REGISTER);
        if (status & 0x01)
            return 1;
        if (!(status & 0x80) && (status & 0x08))
            break;
    }
	
    // Send command
	outsw(port + DATA, (uint16_t *) cmd, 6);

    // Poll until ready
    while (1) {
        uint8_t status = inb(port + COMMAND_REGISTER);
        if (status & 0x01)
            return 1;
        if (!(status & 0x80) && (status & 0x08))
            break;
    }

    if (inb(port + COMMAND_REGISTER) & 1) {
        // error occurred, assume there's no disc here
        return 0;
    }

    int size = inb(port + LBA_HIGH) << 8
		           | inb(port + LBA_MID); // Get the size of transfer

    uint32_t buf[size / 4];

    // Read it
    insw(port + DATA, (uint16_t*)buf, size / 2); 

	return buf[0] + 1;
}


// standard vfs interface
uint32_t atapi_block(unsigned char* data, const struct vfs_block_device* blockdevice, const uint8_t write, const uint32_t index) { 
    if (write) {
        // we don't support writing to the drive yet
        return 0;
    } else {
        // we are reading
        if (blockdevice->ispartition) {
            read_cdrom((struct ata_device*)blockdevice->extraa, index, 1, (uint16_t*)data);
        } else {
            // this is not a partition table driver
            return 0;
        }
    }
    return CDROM_SECTOR_SIZE;
}

void atapi_init(void) {
    // here we register all atapi devices as block devices
    for (int i = 0; i < 4; i++) {
        if (atadevices[i].type == ATA_TYPE_ATAPI) {
            struct vfs_block_device tempdevice;

            tempdevice.block = &atapi_block;
            tempdevice.blocksize = CDROM_SECTOR_SIZE;
            tempdevice.blocks = get_cdrom_capacity_polling(&atadevices[i]);
            tempdevice.mounted = 0;
            tempdevice.isremovable = 1;

            char* name = (char*)kmalloc(16);
            sprintf(name, "atapi%i", i);
            tempdevice.name = name;

            if (tempdevice.blocks) {
                tempdevice.hasmedia = 1;
                terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
                printf("[ ");
                terminal_setfgcolor(VGA_COLOR_GREEN);
                printf("OK ");
                terminal_setfgcolor(VGA_COLOR_LIGHT_MAGENTA);
                printf("] [ATAPI] Found disc in ATA device %i\n", i);
                terminal_setfgcolor(VGA_COLOR_LIGHT_GREY);
            }

            tempdevice.ispartition = 2; // this will be managed by the VFS in the future

            tempdevice.extraa = (void*)&atadevices[i];

            vfs_register_blockdevice(&tempdevice);
        }
    }
}
