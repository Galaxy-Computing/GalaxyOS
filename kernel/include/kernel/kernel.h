#ifndef _KERNEL_KERNEL_H
#define _KERNEL_KERNEL_H

// K_VERSION = "(os name) (release name) (short version)"
// short version = major.minor.build-branch
#define K_OSNAME        "GalaxyOS"
#define K_RELEASE       "Neptune"
#define K_VERSION_SHORT "0.1.20-dev"
#define K_VERSION       K_OSNAME " " K_RELEASE " " K_VERSION_SHORT

#define K_SYSVOLNAME    "local"

extern int kmode; // 0 = initializing (no scheduler or interrupts), 1 = fully running

#endif