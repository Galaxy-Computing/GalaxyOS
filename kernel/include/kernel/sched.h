#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <stdint.h>

uint32_t sched_create_thread(uint32_t ownerpid, uint32_t entrypoint);
uint32_t sched_create_process(uint8_t privilege, uint8_t priority, char* name);
void sched_init(void);
struct thread *sched_loop(void);
void sched_pick_next(void);

extern uint32_t currenttid;
extern uint32_t currentpid;

#endif