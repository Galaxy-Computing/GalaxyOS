// PS2 Driver (ps2.c)
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
#ifdef PS2

#include <kernel/ps2.h>
#include <kernel/io.h>
#include <kernel/exception.h>
#include <kernel/klog.h>
#include <stdio.h>

int timeout_counter = 0;
int dual_channels = 0;
char first_port_fail = 0;
char second_port_fail = 0;
int port1_device = 0;
int port2_device = 0;
int recieved;

void ps2_send_command(char command) {
    outb(PS2_COMMAND, command);
    timeout_counter = 0;
    while ((inb(PS2_COMMAND) & 2) == 2) {
        timeout_counter++;
        if (timeout_counter >= PS2_TIMEOUT_CYCLES) {
            // fail
            log_fail("[PS2] Controller did not respond. Halting!");
            halt();
        }
    }
}

char ps2_send_command_response(char command) {
    outb(PS2_COMMAND, command);
    timeout_counter = 0;
    while ((inb(PS2_COMMAND) & 2) == 2) { 
        timeout_counter++;
        if (timeout_counter >= PS2_TIMEOUT_CYCLES) {
            // fail
            log_fail("[PS2] Controller did not respond. Halting!");
            halt();
        }
    }
    timeout_counter = 0;
    while ((inb(PS2_COMMAND) & 1) == 0) { 
        timeout_counter++;
        if (timeout_counter >= PS2_TIMEOUT_CYCLES) {
            // fail
            log_fail("[PS2] Controller did not respond. Halting!");
            halt();
        }
    }
    return inb(PS2_DATA);
}

int ps2_send_data(char data) {
    timeout_counter = 0;
    while ((inb(PS2_COMMAND) & 2) == 2) {
        timeout_counter++;
        if (timeout_counter >= PS2_TIMEOUT_CYCLES) {
            // fail
            return 0;
        }
    }
    outb(PS2_DATA, data);
    return 1;
}

int ps2_recieve_data() {
    timeout_counter = 0;
    while ((inb(PS2_COMMAND) & 1) == 0) {
        timeout_counter++;
        if (timeout_counter >= PS2_TIMEOUT_CYCLES) {
            // fail
            return -1;
        }
    }
    return (int)inb(PS2_DATA);
}

int ps2_send_data_port2(char data) {
    outb(PS2_COMMAND, 0xD4);
    timeout_counter = 0;
    while ((inb(PS2_COMMAND) & 2) == 2) {
        timeout_counter++;
        if (timeout_counter >= PS2_TIMEOUT_CYCLES) {
            // fail
            return 0;
        }
    }
    outb(PS2_DATA, data);
    timeout_counter = 0;
    while ((inb(PS2_COMMAND) & 2) == 2) {
        timeout_counter++;
        if (timeout_counter >= PS2_TIMEOUT_CYCLES) {
            // fail
            return 0;
        }
    }
    return 1;
}

void ps2_init(void) {
    // TODO: Once we have ACPI, actually check if the controller exists rather than assuming it does

    // Disable ports
    ps2_send_command(0xAD); // Disable 1st port
    ps2_send_command(0xA7); // Disable 2nd port

    // Clear output buffer
    while (((inb(PS2_COMMAND) >> 0) & 1) == 1) inb(PS2_DATA);

    // Self test
    if (ps2_send_command_response(0xAA) != 0x55) {
        // fail
        log_fail("[PS2] PS2 controller self test failed. Halting!");
        halt();
    }

    // Set controller configuration byte
    ps2_send_command(0x60);
    ps2_send_data(0b00100110);

    // Test for dual channels
    ps2_send_command(0xA8);
    if ((ps2_send_command_response(0x20) & 0b00100000) == 0) {
        dual_channels = 1;
        ps2_send_command(0xA7);
        ps2_send_command(0x60);
        ps2_send_data(0b00000100);
    }

    // Interface tests
    first_port_fail = ps2_send_command_response(0xAB);
    if (dual_channels) {
        second_port_fail = ps2_send_command_response(0xA9);
        if (first_port_fail && second_port_fail) {
            log_fail("[PS2] All ports test failed. Halting!");
            halt();
        }
    } else if (first_port_fail) {
        log_fail("[PS2] All ports test failed. Halting!");
        halt();
    }

    // Enable devices
    if (!first_port_fail) {
        ps2_send_command(0xAE);
    }
    if (dual_channels && !second_port_fail) {
        ps2_send_command(0xA8);
    }
    if (dual_channels) {
        if (!first_port_fail && !second_port_fail) {
            ps2_send_command(0x60);
            ps2_send_data(0b00000111);
        } else if (!first_port_fail) {
            ps2_send_command(0x60);
            ps2_send_data(0b00100101);
        } else {
            ps2_send_command(0x60);
            ps2_send_data(0b00010110);
        }
    } else {
        ps2_send_command(0x60);
        ps2_send_data(0b00100101);
    }

    // Reset devices
    if (!first_port_fail) {
        ps2_send_data(0xFF);
        recieved = ps2_recieve_data();
        if (recieved != -1) {
            if (recieved == 0xFC) {
                // we failed it
                port1_device = PS2_DEVICE_NOT_PRESENT;
                log_fail("[PS2] Port 1 device failed self test.");
            } else {
                ps2_recieve_data();
                recieved = ps2_recieve_data();
                if (recieved == -1) {
                    // This is an ancient AT keyboard
                    port1_device = PS2_DEVICE_KEYBOARD;
                    log_ok("[PS2] Detected keyboard in port 1.");
                } else if (recieved == 0xAB) {
                    // This is also a keyboard
                    port1_device = PS2_DEVICE_KEYBOARD;
                    log_ok("[PS2] Detected keyboard in port 1.");
                    ps2_recieve_data(); // We don't care what kind of keyboard it is
                } else if (recieved == 0xAC) {
                    // This is also a keyboard but it's special
                    port1_device = PS2_DEVICE_KEYBOARD;
                    log_ok("[PS2] Detected keyboard in port 1.");
                    ps2_recieve_data(); // No
                } else if (recieved == 0) {
                    // This is a mouse
                    port1_device = PS2_DEVICE_MOUSE;
                    log_ok("[PS2] Detected mouse in port 1. (2 button)");
                } else if (recieved == 0x03) {
                    // This is a better mouse
                    port1_device = PS2_DEVICE_MOUSE_SCROLL;
                    log_ok("[PS2] Detected mouse in port 1. (scroll)");
                } else if (recieved == 0x04) {
                    // This is an even better mouse
                    port1_device = PS2_DEVICE_MOUSE_5;
                    log_ok("[PS2] Detected mouse in port 1. (5 button)");
                }
            }
        } else {
            // probably nothing here
            port1_device = PS2_DEVICE_NOT_PRESENT;
            log_ok("[PS2] Detected nothing in port 1.");
        }
    }
    if (dual_channels && !second_port_fail) {
        ps2_send_data_port2(0xFF);
        recieved = ps2_recieve_data();
        if (recieved != -1) {
            if (recieved == 0xFC) {
                // we failed it
                port2_device = PS2_DEVICE_NOT_PRESENT;
                log_fail("[PS2] Port 2 device failed self test.");
            } else {
                ps2_recieve_data();
                recieved = ps2_recieve_data();
                if (recieved == -1) {
                    // This is an ancient AT keyboard
                    port2_device = PS2_DEVICE_KEYBOARD;
                    log_ok("[PS2] Detected keyboard in port 2.");
                } else if (recieved == 0xAB) {
                    // This is also a keyboard
                    port2_device = PS2_DEVICE_KEYBOARD;
                    log_ok("[PS2] Detected keyboard in port 2.");
                    ps2_recieve_data(); // We don't care what kind of keyboard it is
                } else if (recieved == 0xAC) {
                    // This is also a keyboard but it's special
                    port2_device = PS2_DEVICE_KEYBOARD;
                    log_ok("[PS2] Detected keyboard in port 2.");
                    ps2_recieve_data(); // No
                } else if (recieved == 0) {
                    // This is a mouse
                    port2_device = PS2_DEVICE_MOUSE;
                    log_ok("[PS2] Detected mouse in port 2. (2 button)");
                } else if (recieved == 0x03) {
                    // This is a better mouse
                    port2_device = PS2_DEVICE_MOUSE_SCROLL;
                    log_ok("[PS2] Detected mouse in port 2. (scroll)");
                } else if (recieved == 0x04) {
                    // This is an even better mouse
                    port2_device = PS2_DEVICE_MOUSE_5;
                    log_ok("[PS2] Detected mouse in port 2. (5 button)");
                }
            }
        } else {
            // probably nothing here
            port2_device = PS2_DEVICE_NOT_PRESENT;
            log_ok("[PS2] Detected nothing in port 2.");
        }
    }
}

#endif
