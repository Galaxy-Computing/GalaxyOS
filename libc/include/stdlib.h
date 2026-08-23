#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <sys/cdefs.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((__noreturn__))
void abort(void);
__attribute__((__noreturn__))
void exit(int status);

char* itoa(int num, char* str, int base);
int atoi(char *s);
int abs(int num);
void *malloc(size_t);
void *realloc(void *, size_t);
void *calloc(size_t, size_t);
void free(void *);

// these are not implemented, just here to make libgcc build without errors
int atexit(void (*func)(void));
char *getenv(const char *name);


#ifdef __cplusplus
}
#endif

#endif
