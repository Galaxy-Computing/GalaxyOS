// PS2 Keyboard Driver (ps2kb.c)
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
#ifdef PS2
#ifdef PS2KB

#include <kernel/irq.h>
#include <kernel/idt.h>
#include <kernel/ps2.h>
#include <kernel/kernel.h>
#include <kernel/klog.h>
#include <kernel/ps2kb.h>
#include <kernel/exception.h>
#include <stdio.h>
#include <stdbool.h>

char scancodelower[] = {
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,'\t','`',0,
    0,0,0,0,
    0,'q','1',0,
    0,0,'z','s',
    'a','w','2',0,
    0,'c','x','d',
    'e','4','3',0,
    0,' ','v','f',
    't','r','5',0,
    0,'n','b','h',
    'g','y','6',0,
    0,0,'m','j',
    'u','7','8',0,
    0,',','k','i',
    'o','0','9',0,
    0,'.','/','l',
    ';','p','-',0,
    0,0,'\'',0,
    '[','=',0,0,
    0,0,'\n',']',
    0,'\\',0,0,
    0,0,0,0,
    0,0,0x7F,0,
    0,'1',0,'4',
    '7',0,0,0,
    '0','.','2','5',
    '6','8',0,0,
    0,'+','3','-',
    '*','9',0,0
};

char scancodeupper[] = {
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,'\t','~',0,
    0,0,0,0,
    0,'Q','!',0,
    0,0,'Z','S',
    'A','W','@',0,
    0,'C','X','D',
    'E','$','#',0,
    0,' ','V','F',
    'T','R','%',0,
    0,'N','B','H',
    'G','Y','^',0,
    0,0,'M','J',
    'U','&','*',0,
    0,'<','K','I',
    'O',')','(',0,
    0,'>','?','L',
    ':','P','_',0,
    0,0,'\"',0,
    '{','+',0,0,
    0,0,'\n','}',
    0,'|',0,0,
    0,0,0,0,
    0,0,0x7F,0,
    0,'1',0,'4',
    '7',0,0,0,
    '0','.','2','5',
    '6','8',0,0,
    0,'+','3','-',
    '*','9',0,0
};

bool keydown[256];
unsigned char key;
int state = 0;

unsigned char port;

#define KEYBUF_SIZE 1024
char keybuf[KEYBUF_SIZE];
unsigned int keybuf_end;
unsigned int keybuf_start;

#define COMMAND_QUEUE_SIZE 1024
unsigned char command_queue[COMMAND_QUEUE_SIZE];
unsigned int command_queue_end;
unsigned int command_queue_start;

bool is_sending_command;

void ps2kb_send_command(unsigned char byte) {
    if (((command_queue_end + 1) % COMMAND_QUEUE_SIZE) == command_queue_start) {
        // the buffer is full, we can't really continue
        log_warn("[PS2KB] Command queue is full. The command will be skipped.");
        return;
    }
    command_queue[command_queue_end++] = byte;
    command_queue_end = command_queue_end % COMMAND_QUEUE_SIZE;
}

void ps2kb_resend_command(void) {
    if (port == 1) {
        is_sending_command = true;
        ps2_send_data(command_queue[command_queue_start]);
    } else if (port == 2) {
        is_sending_command = true;
        ps2_send_data_port2(command_queue[command_queue_start]);
    } else if (port == 3) {
        return;
    }
}

void ps2kb_succeed_command(void) {
    command_queue_start++;
    command_queue_start = command_queue_start % COMMAND_QUEUE_SIZE;
    is_sending_command = false;
}

// TODO: this may break with more than one keyboard plugged into the PS/2 ports
void ps2kb_handler(struct regs *r) {
    // We have a keyboard int
    key = ps2_recieve_data();
    if (kmode) {
        if (key == 0xFA) {
            ps2kb_succeed_command();
            return;
        }
        if (key == 0xFE) {
            ps2kb_resend_command();
            return;
        }
        if (!state) {
            if (key == 0xF0) {
                state = 1;
            } else if (key == 0xE0) {
                state = 2;
            } else {
                if (key == 0x58) { keydown[key] = !keydown[key]; }
                else { keydown[key] = true; }
                if (keydown[0x12] || keydown[0x59] || keydown[0x58]) {
                    if (scancodeupper[key]) keybuf[keybuf_end++] = scancodeupper[key];
                } else {
                    if (scancodelower[key]) keybuf[keybuf_end++] = scancodelower[key];
                }
                keybuf_end = keybuf_end % KEYBUF_SIZE;
            }
        } else if (state == 1) {
            if (key != 0x58) keydown[key] = false;
            state = 0;
        } else {
            state = 0;
        }
    }
}

size_t ps2kb_read_chars(char *buf, size_t size) {
    size_t chars_read = 0;
    for (unsigned int i = keybuf_start; i < keybuf_end; i++) {
        if (i-keybuf_start > size) {
            break;
        }
        buf[chars_read++] = keybuf[keybuf_start++];
        keybuf_start = keybuf_start % KEYBUF_SIZE;
    }
    return chars_read;
}

void ps2kb_clear_keybuf(void) {
    keybuf_end = 0;
    keybuf_start = 0;
}

void ps2kb_loop(void) {
    if ((command_queue_end != command_queue_start) && !is_sending_command) {
        ps2kb_resend_command();
    }
}

void ps2kb_init(void) {
    port = 0;
    if (port1_device == PS2_DEVICE_KEYBOARD) {
        irq_install_handler(1, &ps2kb_handler); 
        port |= 1;
    }
    if (port2_device == PS2_DEVICE_KEYBOARD) {
        irq_install_handler(12, &ps2kb_handler);
        port |= 2;
    }
    if (port == 3) {
        log_fail("[PS2KB] Two PS/2 keyboards are currently not supported. The driver will not initialize.");
        irq_uninstall_handler(1); // p1
        irq_uninstall_handler(12); // p2
        is_sending_command = true;
        return;
    }
    keybuf_end = 0;
    keybuf_start = 0;
    command_queue_end = 0;
    command_queue_start = 0;
    is_sending_command = false;
    keydown[0x58] = false;

    ps2kb_send_command(0xF0); // set scancode set
    ps2kb_send_command(0x02); // 2

    ps2kb_send_command(0xED); // set LEDs
    ps2kb_send_command(0x00); // all off

    log_ok("[PS2KB] Initialized keyboards.");
}

#endif
#endif
