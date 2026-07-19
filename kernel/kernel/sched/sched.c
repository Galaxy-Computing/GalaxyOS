// Task Scheduler (sched.c)
// Copyright (C) 2025-2026 Skye310 (Galaxy Computing)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <kernel/liballoc.h>
#include <kernel/exception.h>
#include <kernel/sched.h>
#include <kernel/kernel.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/idle.h>
#include <stdint.h>
#include <string.h>

#define PAGE_DIRECTORY_ADDR 0xFFFFF000

#define THREAD_STATE_RUNNING 0
#define THREAD_STATE_SUSPENDED 1
#define THREAD_STATE_WAITING 2
#define THREAD_STATE_STARTING 3
#define THREAD_STATE_EXITING 4
#define THREAD_STATE_DEAD 5

#define REALLOC_INCREMENT 512
#define DEFAULT_PROCESS_LIST_SIZE REALLOC_INCREMENT*2
#define DEFAULT_THREAD_LIST_SIZE REALLOC_INCREMENT*2
#define MAX_PROCESSES REALLOC_INCREMENT*128
#define MAX_THREADS REALLOC_INCREMENT*256

#define SMALL_REALLOC_INCREMENT 64 // used in the tid list for processes
#define DEFAULT_P_TID_LIST_SIZE SMALL_REALLOC_INCREMENT*2

struct process **processes;
uint32_t processes_size;
uint32_t last_pid;

struct thread **threads;
uint32_t last_tid;
uint32_t threads_size;

uint32_t *queue;
uint32_t queue_start;
uint32_t queue_loc;
uint32_t queue_end;

uint32_t currentpid;
uint32_t currenttid;

uint32_t idletid;

uint32_t get_eflags(void) {
    uint32_t flags;
    asm volatile (
        "pushfl\n\t"
        "popl %0\n\t"
        : "=r" (flags) // Output operand: the 'flags' variable receives the value
        :              // No input operands
        : "memory"     // Clobber list: indicates that memory might be changed
    );
    return flags;
}

uint32_t sched_create_thread(uint32_t ownerpid, uint8_t noqueue, uint32_t entrypoint) {
    if (kmode) { asm("cli"); }
    
    if (last_tid-1 >= MAX_THREADS) {
        panic("Out of thread ids"); // todo: reuse dead ids
    }
    if (last_tid-1 >= processes_size) {
        threads = (struct thread**)krealloc((void*)threads, sizeof(struct thread*) * (threads_size + REALLOC_INCREMENT));
        threads_size += REALLOC_INCREMENT;
    }

    threads[last_tid-1] = (struct thread*)kmalloc(sizeof(struct thread));
    threads[last_tid-1]->pid = ownerpid;
    threads[last_tid-1]->state = THREAD_STATE_STARTING;
    threads[last_tid-1]->cr3 = processes[ownerpid-1]->cr3;
    
    // create the stack
    threads[last_tid-1]->esp_k = (uint32_t*)((uint32_t)kmalloc(4096*4)+4096*4); // this is for context switches and kernel level threads

    if (processes[ownerpid-1]->privilege_level == 0) {
        // set up the stack frame that the irq routine expects
        threads[last_tid-1]->esp_k -= 17;
        threads[last_tid-1]->esp_k[0] = 0x10;
        threads[last_tid-1]->esp_k[1] = 0x10;
        threads[last_tid-1]->esp_k[2] = 0x10;
        threads[last_tid-1]->esp_k[3] = 0x10;
        threads[last_tid-1]->esp_k[4] = 0;
        threads[last_tid-1]->esp_k[5] = 0;
        threads[last_tid-1]->esp_k[6] = 0;
        threads[last_tid-1]->esp_k[7] = (uint32_t)threads[last_tid-1]->esp_k+14;
        threads[last_tid-1]->esp_k[8] = 0;
        threads[last_tid-1]->esp_k[9] = 0;
        threads[last_tid-1]->esp_k[10] = 0;
        threads[last_tid-1]->esp_k[11] = last_tid-1;
        threads[last_tid-1]->esp_k[14] = entrypoint;
        threads[last_tid-1]->esp_k[15] = 0x08;
        threads[last_tid-1]->esp_k[16] = get_eflags();
    } else {
        panic("User space not implemented");
    }

    // add the thread to the process
    if (processes[ownerpid-1]->threadcount >= threads_size) {
        processes[ownerpid-1]->tids = (uint32_t*)krealloc((void*)processes[ownerpid-1]->tids, sizeof(uint32_t) * (processes[ownerpid-1]->tids_size + SMALL_REALLOC_INCREMENT));
        processes[ownerpid-1]->tids_size += SMALL_REALLOC_INCREMENT;
    }
    processes[ownerpid-1]->tids[processes[ownerpid-1]->threadcount++] = last_tid-1;
    
    if (!noqueue) {
        queue[queue_end] = last_tid;
        queue_end++;
        queue_end = queue_end % processes_size;
    }

    if (kmode) { asm("sti"); }

    return last_tid++;
}

uint32_t sched_create_process(uint8_t privilege, const char* name) {
    if (kmode) { asm("cli"); }
    if (last_pid-1 >= MAX_PROCESSES) {
        panic("Out of process ids"); // todo: reuse dead ids
    }
    if (last_pid-1 >= processes_size) {
        processes = (struct process**)krealloc((void*)processes, sizeof(struct process*) * (processes_size + REALLOC_INCREMENT));
        queue = (uint32_t*)krealloc((void*)queue, sizeof(uint32_t) * (processes_size + REALLOC_INCREMENT));

        processes_size += REALLOC_INCREMENT;
    }
    processes[last_pid-1] = (struct process*)kmalloc(sizeof(struct process));
    processes[last_pid-1]->name = (char*)kmalloc(strlen(name));
    memcpy(processes[last_pid-1]->name,name,strlen(name));
    processes[last_pid-1]->privilege_level = privilege;
    processes[last_pid-1]->threadcount = 0;
    processes[last_pid-1]->pid = last_pid;
    processes[last_pid-1]->tids = (uint32_t*)kmalloc(sizeof(uint32_t) * DEFAULT_P_TID_LIST_SIZE);
    processes[last_pid-1]->tids_size = DEFAULT_P_TID_LIST_SIZE;
    processes[last_pid-1]->cr3_virt = (uint32_t*)liballoc_alloc(1); // this gives us a page aligned 4k block of memory for our page directory
    processes[last_pid-1]->cr3 = (uint32_t*)vmm_get_physaddr((address_t)processes[last_pid-1]->cr3_virt);

    // copy the kernel directory entries into the process page directory
    unsigned long *pd = (unsigned long *)PAGE_DIRECTORY_ADDR;
    for (int i = 768; i < 1024; i++) {
        processes[last_pid-1]->cr3_virt[i] = pd[i];
    }
    
    if (kmode) { asm("sti"); }
    return last_pid++;
}

uint32_t sched_set_cr3(uint32_t pid, uint32_t* newcr3) {
    liballoc_free((void*)processes[pid-1]->cr3_virt, 1);
    processes[pid-1]->cr3 = newcr3;
    return pid;
}

uint32_t sched_create_process_idle(void) {
    uint32_t idlepid = sched_create_process(0, "(idle)");
    idletid = sched_create_thread(idlepid, 1, (uint32_t)&idle_loop);
    return idlepid;
}

uint32_t sched_find_next_tid(void) {
    if (queue_end-queue_loc > 0) {
        return queue[queue_loc];
    }
    return 0; // no threads exist
}

uint32_t sched_pop_next_tid(void) {
    if (queue_end-queue_loc > 0) {
        return queue[queue_loc++];
    }
    return 0; // no processes exist
}

void sched_pick_next(void) {
    currentpid = 0;
    currenttid = 0;
    while (!currenttid) {
        uint32_t nexttid = sched_pop_next_tid();
        if (!nexttid) {
            // refresh the queue
            queue_loc = queue_start;
            nexttid = sched_pop_next_tid();
            if (!nexttid) {
                nexttid = idletid;
            }
        }
        switch (threads[nexttid-1]->state) {
            case THREAD_STATE_RUNNING:
                currenttid = nexttid;
                break;
            case THREAD_STATE_SUSPENDED:
                // skip this one
                break;
            case THREAD_STATE_WAITING:
                // also skip it
                break;
            case THREAD_STATE_STARTING:
                // if it's a kernel process, we have nothing to do here
                if (threads[nexttid-1]->privilege_level == 0) { threads[nexttid-1]->state = THREAD_STATE_RUNNING; }
                // if it's not, we need to wait for the process loader to finish spawning the process
                break;
            case THREAD_STATE_EXITING:
                break;
            case THREAD_STATE_DEAD:
                // this really shouldn't happen, a dead process should never be in the queue
                break;
        }
    }
}

struct thread *sched_loop(void) {
    while (!currenttid) {
        sched_pick_next();
    }
    if (threads[currenttid-1]->privilege_level) {
        panic("User space process not implemented");
    }
    return threads[currenttid-1];
}

void sched_init(void) {
    // set up dynamic arrays
    processes = (struct process**)kmalloc(sizeof(struct process) * DEFAULT_PROCESS_LIST_SIZE);
    threads = (struct thread**)kmalloc(sizeof(struct thread) * DEFAULT_THREAD_LIST_SIZE);
    queue = (uint32_t*)kmalloc(sizeof(uint32_t) * DEFAULT_THREAD_LIST_SIZE);
    queue_start = 0;
    queue_loc = 0;
    queue_end = 0;

    processes_size = DEFAULT_PROCESS_LIST_SIZE;
    threads_size = DEFAULT_THREAD_LIST_SIZE;
    last_pid = 1;
    last_tid = 1;

    currentpid = 0;
    currenttid = 0;

    sched_create_process_idle();
}

void set_thread_stack(uint32_t *esp_k) {
    threads[currenttid-1]->esp_k = esp_k;
}