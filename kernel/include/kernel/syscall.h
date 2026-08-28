#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#define SYSCALL_EXIT   1
#define SYSCALL_WAIT   2
#define SYSCALL_READ   3
#define SYSCALL_WRITE  4
#define SYSCALL_OPEN   5
#define SYSCALL_CLOSE  6
#define SYSCALL_EXEC   7
#define SYSCALL_BRK    8
#define SYSCALL_SIZE   9
#define SYSCALL_ERET   10
#define SYSCALL_EHNDLR 11
#define SYSCALL_GETENV 12
#define SYSCALL_SETENV 13
#define SYSCALL_SETUP  14
#define SYSCALL_GETVER 15
#define SYSCALL_CHDIR  16
#define SYSCALL_LIST   17
#define SYSCALL_GETCWD 18

void syscall_init(void);

#endif