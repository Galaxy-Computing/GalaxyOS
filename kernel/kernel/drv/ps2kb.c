#include <kernel/irq.h>
#include <kernel/idt.h>
#include <kernel/ps2.h>
#include <kernel/kernel.h>
#include <kernel/klog.h>
#include <kernel/ps2kb.h>
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
    'u',0,'7','8',
    0,',','k','i',
    'o','0','9',0,
    0,'.','/','l',
    ';','p','-',0,
    0,0,'\'',0,
    '[','=',0,0,
    0,0,0,']',
    0,'\\',0,0,
    0,0,0,0,
    0,0,0x7F,0,
    0,'1',0,'4',
    '7',0,0,0,
    0,'.','2','5',
};

bool keydown[256];
char key;
int state = 0;

void ps2kb_handler(struct regs *r) {
    // We have a keyboard int
    key = ps2_recieve_data();
    if (0) {
        if (state) {
            if (key == 0xF0) {
                state = 1;
            } else if (key == 0xE0) {
                state = 2;
            } else {
                keydown[key] = true;

            }
        } else if (state == 1) {
            keydown[key] = false;
            state = 0;
        } else {
            state = 0;
        }
    }
}

void ps2kb_init() {
    if (port1_device == PS2_DEVICE_KEYBOARD) irq_install_handler(1, &ps2kb_handler);
    if (port2_device == PS2_DEVICE_KEYBOARD) irq_install_handler(12, &ps2kb_handler);
    log_ok("[PS2KB] Installed interrupt handlers.");
}