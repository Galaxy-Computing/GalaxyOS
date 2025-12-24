// Task Scheduler (sched.c)
// Copyright (C) 2025 Skye310 (Galaxy Computing)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <kernel/liballoc.h>
#include <kernel/exception.h>
#include <kernel/sched.h>
#include <stdint.h>
#include <string.h>

#define PROCESS_STATE_RUNNING 0
#define PROCESS_STATE_SUSPENDED 1
#define PROCESS_STATE_WAITING 2
#define PROCESS_STATE_STARTING 3
#define PROCESS_STATE_EXITING 4
#define PROCESS_STATE_DEAD 5

#define REALLOC_INCREMENT 512
#define DEFAULT_PROCESS_LIST_SIZE REALLOC_INCREMENT*2
#define DEFAULT_THREAD_LIST_SIZE REALLOC_INCREMENT*2
#define MAX_PROCESSES REALLOC_INCREMENT*128
#define MAX_THREADS REALLOC_INCREMENT*256

#define SMALL_REALLOC_INCREMENT 64 // used in the tid list for processes
#define DEFAULT_P_TID_LIST_SIZE SMALL_REALLOC_INCREMENT*2

struct process {
    char *name;
    //char *args[]; will do later
    uint32_t threadcount;
    uint32_t *tids; // this is a dynamic array
    uint32_t tids_size;
    uint32_t *mapping_phys; // this will be realloc'd to whatever size we need, maximum 3mb
    uint32_t *mapping_virt; // this will be realloc'd to whatever size we need, maximum 3mb
    uint32_t mapping_size;
    uint32_t pid;
    uint32_t entrypoint;

    uint8_t privilege_level;
    uint8_t state;
    uint8_t priority;
};

struct thread {
    uint32_t eip;
    uint32_t esp;
    uint32_t pid;
    uint8_t state;
};

struct process **processes;
uint32_t processes_size;
uint32_t last_pid;

struct thread **threads;
uint32_t last_tid;
uint32_t threads_size;

uint32_t *max_queue;
uint32_t max_queue_start;
uint32_t max_queue_end;
uint32_t *high_queue;
uint32_t high_queue_start;
uint32_t high_queue_end;
uint32_t *med_queue;
uint32_t med_queue_start;
uint32_t med_queue_end;
uint32_t *low_queue;
uint32_t low_queue_start;
uint32_t low_queue_end;

uint32_t currentpid;
uint32_t currenttid;

uint32_t currentthreadi;

uint32_t sched_create_thread(uint32_t ownerpid, uint32_t entrypoint) {
    asm("cli");
    
    if (last_tid-1 >= MAX_THREADS) {
        panic("Out of thread ids"); // todo: reuse dead ids
    }
    if (last_tid-1 >= processes_size) {
        threads = (struct thread**)krealloc((void*)threads, sizeof(struct thread*) * (threads_size + REALLOC_INCREMENT));
        threads_size += REALLOC_INCREMENT;
    }

    threads[last_tid-1] = (struct thread*)kmalloc(sizeof(struct thread));
    threads[last_tid-1]->pid = ownerpid;
    threads[last_tid-1]->eip = entrypoint;

    // create the stack
    if (processes[ownerpid-1]->privilege_level == 0) {
        // allocate it in kernel space
        threads[last_tid-1]->esp = (uint32_t)kmalloc(4096*4);
    } else {
        panic("User space not implemented");
    }

    // add the thread to the process
    if (processes[ownerpid-1]->threadcount >= threads_size) {
        processes[ownerpid-1]->tids = (uint32_t*)krealloc((void*)processes[ownerpid-1]->tids, sizeof(uint32_t) * (processes[ownerpid-1]->tids_size + SMALL_REALLOC_INCREMENT));
        processes[ownerpid-1]->tids_size += SMALL_REALLOC_INCREMENT;
    }
    processes[ownerpid-1]->tids[processes[ownerpid-1]->threadcount++] = last_tid-1;
    
    asm("sti");

    return last_tid++;
}

uint32_t sched_create_process(uint8_t privilege, uint8_t priority, char* name) {
    asm("cli");
    if (last_pid-1 >= MAX_PROCESSES) {
        panic("Out of process ids"); // todo: reuse dead ids
    }
    if (last_pid-1 >= processes_size) {
        processes = (struct process**)krealloc((void*)processes, sizeof(struct process*) * (processes_size + REALLOC_INCREMENT));
        max_queue = (uint32_t*)krealloc((void*)max_queue, sizeof(uint32_t) * (processes_size + REALLOC_INCREMENT));
        high_queue = (uint32_t*)krealloc((void*)high_queue, sizeof(uint32_t) * (processes_size + REALLOC_INCREMENT));
        med_queue = (uint32_t*)krealloc((void*)med_queue, sizeof(uint32_t) * (processes_size + REALLOC_INCREMENT));
        low_queue = (uint32_t*)krealloc((void*)low_queue, sizeof(uint32_t) * (processes_size + REALLOC_INCREMENT));
        processes_size += REALLOC_INCREMENT;
    }
    processes[last_pid-1] = (struct process*)kmalloc(sizeof(struct process));
    processes[last_pid-1]->name = (char*)kmalloc(strlen(name));
    memcpy(processes[last_pid-1]->name,name,strlen(name));
    processes[last_pid-1]->privilege_level = privilege;
    processes[last_pid-1]->state = PROCESS_STATE_STARTING;
    processes[last_pid-1]->threadcount = 0;
    processes[last_pid-1]->mapping_size = 0;
    processes[last_pid-1]->pid = last_pid;
    processes[last_pid-1]->tids = (uint32_t*)kmalloc(sizeof(uint32_t) * DEFAULT_P_TID_LIST_SIZE);
    processes[last_pid-1]->tids_size = DEFAULT_P_TID_LIST_SIZE;

    processes[last_pid-1]->priority = priority;
    switch (priority) {
        case 0:
            max_queue[max_queue_end] = last_pid;
            max_queue_end++;
            max_queue_end = max_queue_end % processes_size;
            break;
        case 1:
            high_queue[high_queue_end] = last_pid;
            high_queue_end++;
            high_queue_end = high_queue_end % processes_size;
            break;
        case 2:
            med_queue[med_queue_end] = last_pid;
            med_queue_end++;
            med_queue_end = med_queue_end % processes_size;
            break;
        case 3:
            low_queue[low_queue_end] = last_pid;
            low_queue_end++;
            low_queue_end = low_queue_end % processes_size;
            break;
    }

    // create initial thread
    sched_create_thread(last_pid, processes[last_pid-1]->entrypoint);

    asm("sti");
    return last_pid++;
}

uint32_t sched_find_next_pid(void) {
    if (max_queue_end-max_queue_start > 0) {
        return max_queue[max_queue_start];
    }
    if (high_queue_end-high_queue_start > 0) {
        return high_queue[high_queue_start];
    }
    if (med_queue_end-med_queue_start > 0) {
        return med_queue[med_queue_start];
    }
    if (low_queue_end-low_queue_start > 0) {
        return low_queue[low_queue_start];
    }
    return 0; // no processes exist
}

uint32_t sched_pop_next_pid(void) {
    if (max_queue_end-max_queue_start > 0) {
        return max_queue[max_queue_start++];
    }
    if (high_queue_end-high_queue_start > 0) {
        return high_queue[high_queue_start++];
    }
    if (med_queue_end-med_queue_start > 0) {
        return med_queue[med_queue_start++];
    }
    if (low_queue_end-low_queue_start > 0) {
        return low_queue[low_queue_start++];
    }
    return 0; // no processes exist
}

void sched_pick_next(void) {
    uint32_t nextpid = sched_pop_next_pid();
    if (!nextpid) {
        // we have nothing to do
        return;
    }
    currentpid = nextpid;
    switch (processes[nextpid-1]->state) {
        case PROCESS_STATE_RUNNING:
            if (processes[nextpid-1]->threadcount > 0) {
                currentthreadi = 0;
            }
            break;
        case PROCESS_STATE_SUSPENDED:
            break;
        case PROCESS_STATE_WAITING:
            break;
        case PROCESS_STATE_STARTING:
            // if it's a kernel process, we have nothing to do here
            if (processes[nextpid-1]->privilege_level == 0) { processes[nextpid-1]->state = PROCESS_STATE_RUNNING; }
            // if it's not, we need to wait for the process loader to finish spawning the process
            break;
        case PROCESS_STATE_EXITING:
            break;
        case PROCESS_STATE_DEAD:
            // this really shouldn't happen, a dead process should never be in the queue
            break;
    }
}

void sched_loop(void) {
    if (currentpid) {
        if (currentthreadi < processes[currentpid-1]->threadcount) {
            
            if (processes[currentpid-1]->privilege_level) {
                panic("User space process not implemented");
            }
            // relatively menacing looking line
            //jump_to_address(threads[processes[currentpid-1]->tids[currentthreadi]]->eip, threads[processes[currentpid-1]->tids[currentthreadi++]]->esp);
        }
    }
    sched_pick_next();
}

void sched_init(void) {
    // set up dynamic arrays
    processes = (struct process**)kmalloc(sizeof(struct process) * DEFAULT_PROCESS_LIST_SIZE);
    threads = (struct thread**)kmalloc(sizeof(struct thread) * DEFAULT_THREAD_LIST_SIZE);
    max_queue = (uint32_t*)kmalloc(sizeof(uint32_t) * DEFAULT_PROCESS_LIST_SIZE);
    max_queue_start = 0;
    max_queue_end = 0;
    high_queue = (uint32_t*)kmalloc(sizeof(uint32_t) * DEFAULT_PROCESS_LIST_SIZE);
    high_queue_start = 0;
    high_queue_end = 0;
    med_queue = (uint32_t*)kmalloc(sizeof(uint32_t) * DEFAULT_PROCESS_LIST_SIZE);
    med_queue_start = 0;
    med_queue_end = 0;
    low_queue = (uint32_t*)kmalloc(sizeof(uint32_t) * DEFAULT_PROCESS_LIST_SIZE);
    low_queue_start = 0;
    low_queue_end = 0;

    processes_size = DEFAULT_PROCESS_LIST_SIZE;
    threads_size = DEFAULT_THREAD_LIST_SIZE;
    last_pid = 1;
    last_tid = 1;

    currentpid = 0;
    currenttid = 0;

    // set up our irq handler here
}