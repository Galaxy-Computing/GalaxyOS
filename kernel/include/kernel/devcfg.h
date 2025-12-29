// Use this file to configure which device drivers should be built into the kernel.
// Comment out a line to remove the driver from the build.

#ifndef _KERNEL_DEVCFG_H
#define _KERNEL_DEVCFG_H

// HID drivers
#define PS2     1 // PS2 controller
#define PS2KB   2 // PS2 keyboard

// Storage drivers
#define VFS     3 // Virtual File System
#define RDISK   4 // RAM Disk
#define ATAPIO  5 // ATAPIO interface
#define VFAT    7 // VFAT FS driver

// TTY drivers
#define VGATEXT 6 // VGA text terminal

// Don't modify anything past this line if you are just changing configuration.

#ifdef VGATEXT
#include <kernel/tty.h>
#endif

#ifdef PS2
#include <kernel/ps2.h>
#endif

#ifdef PS2KB
#include <kernel/ps2kb.h>
#endif

#ifdef VFS
#include <kernel/vfs.h>
#endif

#ifdef RDISK
#include <kernel/rdisk.h>
#endif

#ifdef ATAPIO
#include <kernel/ata.h>
#endif

#ifdef VFAT
#include <kernel/vfat.h>
#endif

void devinit(void);
void devinit_tty(void);
void devinit_init(void);

#endif