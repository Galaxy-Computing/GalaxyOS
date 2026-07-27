#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define EOF (-1)
#define SEEK_SET 0

typedef struct _iobuf
{
    int id;
    char* buffer;
    int buffer_size;
    char* ptr;
    int count;
    uint32_t total_size;
    uint8_t mode; // 1 = read 2 = write 4 = append 8 = truncate
    uint8_t flags; // 1 = eof 2 = err
} FILE;

#ifdef __cplusplus
extern "C" {
#endif

extern FILE *stderr;
#define stderr stderr

int printf(const char* __restrict, ...);
int sprintf(char *str, const char* restrict format, ...);
int putchar(int);
int puts(const char*);
int fflush(FILE*);
int fprintf(FILE*, const char*, ...);
FILE *fopen(const char *filename, const char *mode);

#ifdef __cplusplus
}
#endif

#endif
