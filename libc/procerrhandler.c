#include <procerr.h>
#include <stdio.h>

// Boilerplate code for the process error handler.

int inhandler = 0;

char cpumessages[][32] = {
    "Division Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Unknown"
};

int _libc_procerrhandler(int errcode) {
    if (!inhandler) { // hopefully prevents the handler from "triple faulting"
        inhandler = 1;
        printf("Process died: %s\n", cpumessages[errcode-1]);
    }
    inhandler = 0;
    return -1;
}