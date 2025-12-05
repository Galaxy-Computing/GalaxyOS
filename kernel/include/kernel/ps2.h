#ifndef _KERNEL_PS2_H
#define _KERNEL_PS2_H

#define PS2_DATA 0x60
#define PS2_COMMAND 0x64
#define PS2_TIMEOUT_CYCLES 20000

#define PS2_DEVICE_NOT_PRESENT 0
#define PS2_DEVICE_KEYBOARD 1
#define PS2_DEVICE_MOUSE 2
#define PS2_DEVICE_MOUSE_SCROLL 3
#define PS2_DEVICE_MOUSE_5 4

extern int port1_device;
extern int port2_device;

void ps2_init(void);
int ps2_recieve_data(void);
int ps2_send_data(char data);

#endif
