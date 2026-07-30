#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void*, const void*, size_t);
void* memcpy(void* __restrict, const void* __restrict, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
size_t strlen(const char*);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, register size_t n);
char *strncpy (char *s1, const char *s2, size_t n);
char *strtok (char *s, const char *delim);
char *strtok_r (char *s, const char *delim, char **olds);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s1, const char *s2);
char *strpbrk(const char *s, const char *accept);
void *memchr(const void *s, int c, size_t n);
void *__rawmemchr (const void *s, int c);

#ifdef __cplusplus
}
#endif

#endif
