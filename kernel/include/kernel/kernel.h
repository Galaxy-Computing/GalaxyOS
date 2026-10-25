#ifndef _KERNEL_KERNEL_H
#define _KERNEL_KERNEL_H

#include <stddef.h>
#include <stdint.h>

// K_VERSION = "(os name) (release name) (short version)"
// short version = major.minor.build-branch
#define K_OSNAME        "GalaxyOS"
#define K_RELEASE       "Neptune"
#define K_VERSION_SHORT "0.1.27-" GIT_BRANCH
#define K_VERSION       K_OSNAME " " K_RELEASE " " K_VERSION_SHORT

#define K_SYSVOLNAME    "local"

extern int kmode; // 0 = interrupts not supported, 1 = interrupts supported

#define KERNEL_START 0xC0000000

#endif