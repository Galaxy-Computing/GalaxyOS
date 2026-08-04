#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__))
void abort(void);
char* itoa(int num, char* str, int base);
int atoi(char *s);
int abs(int num);

// these are not implemented, just here to make libgcc build without errors
void free(void*);
void* malloc(size_t);
void* calloc(size_t num, size_t size);
int atexit(void (*func)(void));
char *getenv(const char *name);
__attribute__((__noreturn__))
void exit(int status);

#ifdef __cplusplus
}
#endif

#endif
