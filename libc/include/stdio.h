#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)
#define SEEK_SET 0

typedef struct { int unused; } FILE;

#ifdef __cplusplus
extern "C" {
#endif

extern FILE* stderr;
#define stderr stderr

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);
int fflush(FILE*);
int fprintf(FILE*, const char*, ...);

#ifdef __cplusplus
}
#endif

#endif
