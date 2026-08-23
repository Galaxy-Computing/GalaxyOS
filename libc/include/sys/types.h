#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H 1

#include <stdint.h>

typedef uint32_t off_t;
typedef uint32_t blksize_t;
typedef uint32_t blkcnt_t;

typedef long signed int ssize_t;
typedef long unsigned int size_t;

typedef uint32_t mode_t;
typedef int pid_t;

typedef __INTPTR_TYPE__ intptr_t;

#endif