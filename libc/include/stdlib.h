#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__))
void abort(void);
void free(void*);
void* malloc(size_t);
char* itoa(int num, char* str, int base);
int atoi(char *s);

#ifdef __cplusplus
}
#endif

#endif
