#ifndef _KERNEL_IO_H
#define _KERNEL_IO_H

#include <stdint.h>

void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
void io_wait(void);

static inline void insw(uint16_t __port, void *__buf, unsigned long __n) {
	__asm__ __volatile__("cld; rep; insw"
			: "+D"(__buf), "+c"(__n)
			: "d"(__port));
}

static inline void outsw(uint16_t __port, const void *__buf, unsigned long __n) {
	__asm__ __volatile__("cld; rep; outsw"
			: "+S"(__buf), "+c"(__n)
			: "d"(__port));
}

#endif