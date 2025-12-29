#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)
#define SEEK_SET 0

typedef struct _iobuf
{
    // todo: add the fields here. this struct is platform specific, so we can do whatever we want with this.
} FILE;

#ifdef __cplusplus
extern "C" {
#endif

extern FILE *stderr;
#define stderr stderr

int printf(const char* __restrict, ...);
int putchar(int);
int puts(const char*);
int fflush(FILE*);
int fprintf(FILE*, const char*, ...);
FILE *fopen(const char *filename, const char *mode);

#ifdef __cplusplus
}
#endif

#endif
