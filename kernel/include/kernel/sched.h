#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <stdint.h>

struct process {
    char *name;
    //char *args[]; will do later
    uint32_t threadcount;
    uint32_t *tids; // this is a dynamic array
    uint32_t tids_size;
    uint32_t *cr3; // pointer to the page tables
    uint32_t *cr3_virt; // where the process's page directory is mapped in kernel space
    uint32_t pid;
    uint32_t entrypoint;

    uint8_t privilege_level;
    uint8_t state;
    uint8_t priority;
};

struct thread {
    uint32_t *esp_k;
    uint32_t *cr3; // pointer to the page tables (this should match the one in process)
    uint32_t pid;
    uint8_t privilege_level;
    uint8_t state;
} __attribute__((packed)); // this is because this will be accessed from asm

uint32_t sched_create_thread(uint32_t ownerpid, uint32_t entrypoint);
uint32_t sched_create_process(uint8_t privilege, uint8_t priority, const char* name);
void sched_init(void);
struct thread *sched_loop(void);
void sched_pick_next(void);
uint32_t sched_set_cr3(uint32_t pid, uint32_t* newcr3);

extern uint32_t currenttid;
extern uint32_t currentpid;

#endif