#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#define SYSCALL_EXIT   1
#define SYSCALL_READ   3
#define SYSCALL_WRITE  4
#define SYSCALL_OPEN   5
#define SYSCALL_EXEC   7
#define SYSCALL_BRK    8
#define SYSCALL_ERET   10
#define SYSCALL_EHNDLR 11

void syscall_init(void);

#endif