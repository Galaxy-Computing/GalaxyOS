#ifndef _KERNEL_EXCEPTION_H
#define _KERNEL_EXCEPTION_H

void isrs_install(void);

void panic(char *message);
extern void halt(void);

#endif