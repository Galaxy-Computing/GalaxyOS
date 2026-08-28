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
FILE *fopen(const char *filename, const char *mode);

// these are not implemented, just here to make libgcc build without errors
int fflush(FILE*);
int fprintf(FILE*, const char*, ...);
int fclose(FILE* stream);
size_t fread(void* restrict buffer, size_t size, size_t count, FILE* restrict stream);
int fseek(FILE* stream, long offset, int origin);
long ftell(FILE* stream);
size_t fwrite(const void* restrict buffer, size_t size, size_t count, FILE* restrict stream);
void setbuf(FILE* restrict stream, char* restrict buffer);
int vfprintf(FILE* restrict stream, const char* restrict format, va_list vlist);
int feof(FILE *stream);

#ifdef __cplusplus
}
#endif

#endif
