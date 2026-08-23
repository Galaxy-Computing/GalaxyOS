#ifndef _GLCORE_PROCERR_H
#define _GLCORE_PROCERR_H

#include <stdint.h>

#ifndef _KERNEL_SCHED_H
typedef int (*perrhandle)(uint8_t);
#endif

#define PROCERR_DIVZ 1
#define PROCERR_INVO 7
#define PROCERR_ACCV 15
#define PROCERR_KINT 33

#endif