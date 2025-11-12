#include <kernel/irq.h>
#include <kernel/idt.h>
#include <kernel/ps2.h>
#include <kernel/kernel.h>
#include <kernel/klog.h>
#include <kernel/ps2kb.h>
#include <stdio.h>

void ps2kb_handler(struct regs *r) {
    // We have a keyboard int
    if (bootfinished) {
        putchar(ps2_recieve_data());
    }
}

void ps2kb_init() {
    if (port1_device == PS2_DEVICE_KEYBOARD) irq_install_handler(1, &ps2kb_handler);
    if (port2_device == PS2_DEVICE_KEYBOARD) irq_install_handler(12, &ps2kb_handler);
    log_ok("[PS2KB] Installed interrupt handlers.");
}