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
#include <kernel/pload.h>
#include <kernel/term.h>
#include <kernel/irq.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define PAGE_DIRECTORY_ADDR 0xFFFFF000

#define THREAD_STACK_SIZE 4096 * 2

#define REALLOC_INCREMENT 512
#define DEFAULT_PROCESS_LIST_SIZE REALLOC_INCREMENT*2
#define DEFAULT_THREAD_LIST_SIZE REALLOC_INCREMENT*2
#define MAX_PROCESSES REALLOC_INCREMENT*128
#define MAX_THREADS REALLOC_INCREMENT*256

#define SMALL_REALLOC_INCREMENT 64 // used in the tid list for processes
#define DEFAULT_P_TID_LIST_SIZE SMALL_REALLOC_INCREMENT*2

struct process **processes;
int processes_size;
int last_pid;

struct thread **threads;
int last_tid;
int threads_size;

int *queue;
int queue_start;
int queue_loc;
int queue_end;

int currenttid;
struct process *currentps;

int idletid;

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

int sched_create_thread(int ownerpid, uint8_t noqueue, uint32_t entrypoint) {
    if (kmode) { asm("cli"); }
    
    if (last_tid >= MAX_THREADS) {
        panic("Out of thread ids"); // todo: reuse dead ids
    }
    if (last_tid >= processes_size) {
        threads = (struct thread**)krealloc((void*)threads, sizeof(struct thread*) * (threads_size + REALLOC_INCREMENT));
        threads_size += REALLOC_INCREMENT;
    }

    threads[last_tid] = (struct thread*)kmalloc(sizeof(struct thread));
    threads[last_tid]->pid = ownerpid;
    threads[last_tid]->state = THREAD_STATE_STARTING;
    threads[last_tid]->cr3 = processes[ownerpid]->cr3;
    
    // create the stack
    threads[last_tid]->esp_k = (uint32_t*)((uint32_t)kmalloc(THREAD_STACK_SIZE)+THREAD_STACK_SIZE); // this is for context switches and kernel level threads

    if (!processes[ownerpid]->privilege_level) {
        // set up the stack frame that the irq routine expects
        threads[last_tid]->esp_k = (uint32_t*)((uint32_t)(threads[last_tid]->esp_k) - 68);
        threads[last_tid]->esp_k[0] = 0x10; // gs
        threads[last_tid]->esp_k[1] = 0x10; // fs
        threads[last_tid]->esp_k[2] = 0x10; // es
        threads[last_tid]->esp_k[3] = 0x10; // ds
        threads[last_tid]->esp_k[4] = 0; // edi
        threads[last_tid]->esp_k[5] = 0; // esi
        threads[last_tid]->esp_k[6] = 0; // ebp
        threads[last_tid]->esp_k[7] = 0; // esp (this is ignored)
        threads[last_tid]->esp_k[8] = 0; // ebx
        threads[last_tid]->esp_k[9] = 0; // edx
        threads[last_tid]->esp_k[10] = 0; // ecx
        threads[last_tid]->esp_k[11] = last_tid; // eax
        threads[last_tid]->esp_k[12] = 0;
        threads[last_tid]->esp_k[13] = 0; 
        threads[last_tid]->esp_k[14] = entrypoint; // eip
        threads[last_tid]->esp_k[15] = 0x08; // cs
        threads[last_tid]->esp_k[16] = get_eflags(); // eflags
    } else {
        // userland thread
        vmm_map_stack(threads[last_tid]);
        if (threads[last_tid]->stackpdi == -1) { return -1; }

        threads[last_tid]->esp_k = (uint32_t*)((uint32_t)(threads[last_tid]->esp_k) - 76);
        threads[last_tid]->esp_k[0] = 0x20 | 3; // gs
        threads[last_tid]->esp_k[1] = 0x20 | 3; // fs
        threads[last_tid]->esp_k[2] = 0x20 | 3; // es
        threads[last_tid]->esp_k[3] = 0x20 | 3; // ds
        threads[last_tid]->esp_k[4] = 0; // edi
        threads[last_tid]->esp_k[5] = 0; // esi
        threads[last_tid]->esp_k[6] = 0; // ebp
        threads[last_tid]->esp_k[7] = 0; // esp (this is ignored)
        threads[last_tid]->esp_k[8] = 0; // ebx
        threads[last_tid]->esp_k[9] = 0; // edx
        threads[last_tid]->esp_k[10] = 0; // ecx
        threads[last_tid]->esp_k[11] = last_tid; // eax
        threads[last_tid]->esp_k[12] = 0;
        threads[last_tid]->esp_k[13] = 0; 
        threads[last_tid]->esp_k[14] = entrypoint; // eip
        threads[last_tid]->esp_k[15] = 0x18 | 3; // cs
        threads[last_tid]->esp_k[16] = 0b1000000010; // eflags
        threads[last_tid]->esp_k[17] = (threads[last_tid]->stackpdi+1) * 0x400000; // userland esp
        threads[last_tid]->esp_k[18] = 0x20 | 3; // ss
    }
    threads[last_tid]->entrypoint = &threads[last_tid]->esp_k[14];
    threads[last_tid]->privilege_level = processes[ownerpid]->privilege_level;

    // add the thread to the process
    if (processes[ownerpid]->threadcount >= threads_size) {
        processes[ownerpid]->tids = (int*)krealloc((void*)processes[ownerpid]->tids, sizeof(int) * (processes[ownerpid]->tids_size + SMALL_REALLOC_INCREMENT));
        processes[ownerpid]->tids_size += SMALL_REALLOC_INCREMENT;
    }
    processes[ownerpid]->tids[processes[ownerpid]->threadcount++] = last_tid;
    
    if (!noqueue) {
        queue[queue_end] = last_tid;
        queue_end++;
        queue_end = queue_end % processes_size;
    }

    if (kmode) { asm("sti"); }

    return last_tid++;
}

int sched_create_process(uint8_t privilege, const char* name) {
    if (kmode) { asm("cli"); }
    if (last_pid >= MAX_PROCESSES) {
        panic("Out of process ids"); // todo: reuse dead ids
    }
    if (last_pid >= processes_size) {
        processes = (struct process**)krealloc((void*)processes, sizeof(struct process*) * (processes_size + REALLOC_INCREMENT));
        queue = (int*)krealloc((void*)queue, sizeof(int) * (processes_size + REALLOC_INCREMENT));

        processes_size += REALLOC_INCREMENT;
    }
    processes[last_pid] = (struct process*)kmalloc(sizeof(struct process));
    processes[last_pid]->name = (char*)kmalloc(strlen(name)+1);
    memcpy(processes[last_pid]->name,name,strlen(name));
    processes[last_pid]->name[strlen(name)] = '\0';
    processes[last_pid]->argv = NULL; // should implement this later
    processes[last_pid]->argc = 0; // should implement this later
    processes[last_pid]->privilege_level = privilege;
    processes[last_pid]->threadcount = 0;
    processes[last_pid]->pid = last_pid;
    processes[last_pid]->tids = (int*)kmalloc(sizeof(int) * DEFAULT_P_TID_LIST_SIZE);
    processes[last_pid]->tids_size = DEFAULT_P_TID_LIST_SIZE;
    processes[last_pid]->cr3_virt = (uint32_t*)liballoc_alloc(1); // this gives us a page aligned 4k block of memory for our page directory
    memset(processes[last_pid]->cr3_virt, 0, 4096);
    processes[last_pid]->cr3 = (uint32_t*)vmm_get_physaddr((address_t)processes[last_pid]->cr3_virt);

    processes[last_pid]->psregions = kmalloc(sizeof(struct ps_region*) * 64);
    processes[last_pid]->psregions_count = 0;

    processes[last_pid]->openfiles = (struct vfs_file_open**)kmalloc(sizeof(struct vfs_file_open*) * 32);
    processes[last_pid]->openfiles_size = 32;
    processes[last_pid]->openfiles_loc = 0;

    // these two should be changed by the process loader, but we don't need these if the process is in the kernel
    processes[last_pid]->brk = 0;
    processes[last_pid]->brk_offset = 0;
    processes[last_pid]->pgbrk = 0;

    processes[last_pid]->procerr = 0;
    processes[last_pid]->procerrhandler = NULL;
    processes[last_pid]->state = 2;
    processes[last_pid]->waiting_threads = 0;

    processes[last_pid]->exitcode = 0;
    processes[last_pid]->terminal = &defaultterm;

    // copy the kernel directory entries into the process page directory
    unsigned long *pd = (unsigned long *)PAGE_DIRECTORY_ADDR;
    for (int i = 768; i < 1024; i++) {
        processes[last_pid]->cr3_virt[i] = pd[i];
    }

    // change the last page directory entry to map to itself
    processes[last_pid]->cr3_virt[1023] = ((uint32_t)processes[last_pid]->cr3 | 0x3);
    
    if (kmode) { asm("sti"); }
    return last_pid++;
}

int sched_set_cr3(int pid, uint32_t* newcr3) {
    liballoc_free((void*)processes[pid]->cr3_virt, 1);
    processes[pid]->cr3_virt = newcr3;
    processes[pid]->cr3 = (uint32_t*)vmm_get_physaddr((uint32_t)newcr3);
    return pid;
}

uintptr_t sched_setbrk_true(void* addr, struct process *ps) {
    uintptr_t retval = ps->brk;
    if (addr != NULL) {
        uintptr_t realbrk = (uintptr_t)addr + 1; // we add one here because if addr is on a page boundary, the last byte will be unmapped
        if (realbrk % 4096) { realbrk += 4096 - (realbrk % 4096); } // round up to a page boundary
        if (ps->pgbrk == realbrk) { // we already have the proper amount of memory allocated, there's nothing that needs to be done
            ps->brk = (uintptr_t)addr;
            return ps->brk;
        } else {
            // make sure we're in the right page directory
            // this is a bit nasty but i don't want to rewrite a lot of vmm code
            uintptr_t oldcr3;
            uintptr_t cr3 = (uintptr_t)ps->cr3;
            __asm__ (
                "mov %%cr3, %0\n\t"
                "mov %1, %%cr3\n\t"
                : "=r" (oldcr3)
                : "r" (cr3)
            );
            if (ps->pgbrk < realbrk) { // we need to allocate more pages
                int pagecnt = (realbrk - ps->pgbrk) / 4096;
                int i;
                address_t pages[pagecnt];
                
                for (i = 0; i < pagecnt; i++) {
                    pages[i] = pmm_alloc_page();
                }
                ps->brk_offset += realbrk - ps->pgbrk;
                if (vmm_map_pages_u(pages, pagecnt, ps) != NULL) { // this changes pgbrk for us
                    ps->brk = (uintptr_t)addr;
                    retval = ps->brk;
                } 
            }
            if (ps->pgbrk > realbrk) { // we need to free some pages
                int pagecnt = (ps->pgbrk - realbrk) / 4096;
                int i;
                int err = 0;

                for (i = 0; i < pagecnt; i++) {
                    address_t physaddr = vmm_get_physaddr((address_t)realbrk+(i*4096));
                    if (!physaddr) {
                        err = 1;
                        break;
                    }
                    pmm_free_page(physaddr);
                    if (!vmm_unmap_page((address_t)addr+(i*4096))) {
                        err = 1;
                        break;
                    }
                }
                if (!err) {
                    ps->brk_offset -= ps->pgbrk - realbrk;
                    ps->brk = (uintptr_t)addr;
                    retval = ps->brk;
                }
            }
            __asm__ (
                "mov %0, %%cr3\n\t"
                : : "r" (oldcr3)
            );
        }
    }
    return retval;
}

uintptr_t sched_setbrk(void* addr) {
    return sched_setbrk_true(addr, currentps);
}

void sched_free_all(struct process *ps) {
    uintptr_t oldcr3;
    uintptr_t cr3 = (uintptr_t)ps->cr3;
    __asm__ (
        "mov %%cr3, %0\n\t"
        "mov %1, %%cr3\n\t"
        : "=r" (oldcr3)
        : "r" (cr3)
    );
    sched_setbrk_true((void*)(ps->pgbrk-ps->brk_offset), ps);
    for (uint32_t i = 0; i < ps->psregions_count; i++) {
        uint32_t pagecnt = ps->psregions[i]->size / 4096;
        for (uint32_t x = 0; x < pagecnt; x++) {
            address_t physaddr = vmm_get_physaddr((address_t)ps->psregions[i]->offset+(x*4096));
            if (!physaddr) {
                continue;
            }
            pmm_free_page(physaddr);
            vmm_unmap_page((address_t)ps->psregions[i]->offset+(x*4096));
        }
        kfree(ps->psregions[i]);
    }
    kfree(ps->psregions);
    __asm__ (
        "mov %0, %%cr3\n\t"
        : : "r" (oldcr3)
    );
}

int sched_exit_process(int exitcode) {
    currentps->exitcode = exitcode;
    currentps->state = 1;
    for (int i = 0; i < currentps->threadcount; i++) {
        threads[currentps->tids[i]]->state = THREAD_STATE_EXITING;
    }
    return 0;
}

int sched_create_process_idle(void) {
    int idlepid = sched_create_process(0, "(idle)");
    idletid = sched_create_thread(idlepid, 1, (uint32_t)&idle_loop);
    return idlepid;
}

void sched_suspend_thread(uint8_t irq, int tid) {
    if (kmode) { asm("cli"); }
    threads[tid]->wait = irq;
    threads[tid]->state = THREAD_STATE_SUSPENDED;
    if (kmode) { asm("sti"); }
}

void sched_suspend_current_thread(uint8_t irq) {
    sched_suspend_thread(irq, currenttid);
    asm("int $0x30"); // this yields to the next thread
}

int sched_wait_process(int pid) {
    if (pid >= last_pid) {
        return -1;
    }
    if (pid < 0) {
        return -1;
    }
    threads[currenttid]->wait = pid;
    processes[pid]->waiting_threads++;
    threads[currenttid]->state = THREAD_STATE_WAITING;
    return 0;
}

int sched_find_next_tid(void) {
    if (queue_end-queue_loc > 0) {
        return queue[queue_loc];
    }
    return -1; // no threads are left
}

int sched_pop_next_tid(void) {
    if (queue_end-queue_loc > 0) {
        return queue[queue_loc++];
    }
    return -1; // no threads are left
}

void sched_check_suspended_threads(uint8_t irq) {
    for (int i = 0; i < last_tid; i++) {
        if (threads[i]->state == THREAD_STATE_SUSPENDED) {
            if (threads[i]->wait == irq) {
                threads[i]->state = THREAD_STATE_RUNNING;
            }
        }
    }
}

int sched_check_waiting_thread(int tid) {
    int pid = threads[tid]->wait;
    if (processes[pid]->threadcount == 0) {
        processes[pid]->waiting_threads--;
        threads[tid]->wait = 0;
        threads[tid]->state = THREAD_STATE_RUNNING;
        return pid;
    }
    return -1;
}

void sched_pick_next(void) {
    currenttid = -1;
    int loop = 0;
    while (currenttid == -1) {
        int nexttid = sched_pop_next_tid();
        if (nexttid == -1) {
            if (loop) {
                // we have no active threads
                nexttid = idletid;
            }
            // refresh the queue
            queue_loc = queue_start;
            nexttid = sched_pop_next_tid();
            loop = 1;
            if (nexttid == -1) {
                // there are no threads in the queue
                nexttid = idletid;
            }
        }
        switch (threads[nexttid]->state) {
            case THREAD_STATE_RUNNING:
                // this is the next thread we will run
                currentps = processes[threads[nexttid]->pid];
                currenttid = nexttid;
                break;
            case THREAD_STATE_SUSPENDED:
                // skip it
                break;
            case THREAD_STATE_WAITING:
                // check for the process the thread is waiting for
                int pid = sched_check_waiting_thread(nexttid);
                if (pid != -1) {
                    // that process has exited, so the thread is resumed
                    currentps = processes[threads[nexttid]->pid];
                    currenttid = nexttid;
                    threads[nexttid]->esp_k[11] = processes[pid]->exitcode; // set eax to the exitcode of the process

                    // check if there are still any waiting threads
                    if (!processes[threads[nexttid]->pid]->waiting_threads) {
                        // if there aren't any, we can safely free the entire process
                        kfree(processes[threads[nexttid]->pid]->name);
                        if (processes[threads[nexttid]->pid]->argv != NULL) {
                            kfree(processes[threads[nexttid]->pid]->argv);
                        }
                        kfree(processes[threads[nexttid]->pid]);
                        processes[threads[nexttid]->pid] = NULL; // set it to a null pointer just so we know this isn't valid
                    }
                }
                break;
            case THREAD_STATE_STARTING:
                // if it's a kernel thread, we have nothing to do here
                if (threads[nexttid]->privilege_level == 0) { 
                    threads[nexttid]->state = THREAD_STATE_RUNNING; 
                    break;
                }
                // if it's not, we need to check if the process is still starting
                switch (processes[threads[nexttid]->pid]->state) {
                    case PROCESS_STATE_STARTING:
                        // it is, so call the process loader
                        pload_load_process(threads[nexttid]->pid, nexttid);
                        break;
                    case PROCESS_STATE_RUNNING:
                    case PROCESS_STATE_LOADING:
                        // we can safely start the thread
                        threads[nexttid]->state = THREAD_STATE_RUNNING;
                        break;
                }
                break;
            case THREAD_STATE_EXITING:
                if (processes[threads[nexttid]->pid]->state == PROCESS_STATE_EXITING) {
                    // the entire process is exiting, mark everything as dead
                    for (int i = 0; i < processes[threads[nexttid]->pid]->threadcount; i++) {
                        threads[processes[threads[nexttid]->pid]->tids[i]]->state = THREAD_STATE_DEAD; // if a thread is marked dead, it will be freed later
                    }
                    processes[threads[nexttid]->pid]->tids_size = 0;
                    processes[threads[nexttid]->pid]->threadcount = 0;
                    
                    // these aren't needed anymore and can be safely freed
                    kfree(processes[threads[nexttid]->pid]->tids); 
                    liballoc_free(processes[threads[nexttid]->pid]->cr3_virt, 1);

                    // we don't free it if there are any other threads waiting on this process since we need it's exit code
                    if (!processes[threads[nexttid]->pid]->waiting_threads) {
                        // if there aren't any, we can safely free the entire process
                        kfree(processes[threads[nexttid]->pid]->name);
                        if (processes[threads[nexttid]->pid]->argv != NULL) {
                            kfree(processes[threads[nexttid]->pid]->argv);
                        }
                        kfree(processes[threads[nexttid]->pid]);
                        processes[threads[nexttid]->pid] = NULL; // set it to a null pointer just so we know this isn't valid
                    }
                }
                threads[nexttid]->state = THREAD_STATE_DEAD;
                break;
            case THREAD_STATE_DEAD:
                // take it out of the queue
                for (int i = queue_loc-1; i < queue_end-1; i++) {
                    queue[i] = i+1;
                }
                queue_end--;
                queue_loc--;

                // free the thread
                kfree(threads[nexttid]->esp_k);
                kfree(threads[nexttid]);
                threads[nexttid] = NULL; // set it to a null pointer just so we know this isn't valid
                break;
        }
    }
}

struct thread *sched_loop(void) {
    sched_pick_next();
    if (currenttid == -1) {
        panic("sched_pick_next returned -1");
    }
    return threads[currenttid];
}

void sched_user_fault(int eno, uint32_t errorcode) {
    switch (eno) {
        case 0x0E: // it's a page fault
            uintptr_t cr2;
            __asm__(
                "mov %%cr2, %0\n\t"
                : "=r" (cr2)
            );
            if (!(errorcode & 1)) {
                // check if the page fault was in the range for the stack
                if ((cr2 > (threads[currenttid]->stackpdi * 0x400000)) && (cr2 < (threads[currenttid]->stackpdi * 0x400000 + 0x3FFFFF))) {
                    // if it was, allocate the page
                    address_t newpage = pmm_alloc_page();
                    vmm_map_page(newpage, cr2 & 0xFFFFF000, 0x7, 0);
                    return; // the program can continue on
                }
            }
            break;
        case 0x01: // debugger interrupt, we can just ignore this
            return;
    }
    // we couldn't handle the exception if we're down here
    // send it to the process as procerr
    currentps->procerr = eno+1;
}

void sched_init(void) {
    // set up dynamic arrays
    processes = (struct process**)kmalloc(sizeof(struct process) * DEFAULT_PROCESS_LIST_SIZE);
    threads = (struct thread**)kmalloc(sizeof(struct thread) * DEFAULT_THREAD_LIST_SIZE);
    queue = (int*)kmalloc(sizeof(int) * DEFAULT_THREAD_LIST_SIZE);
    queue_start = 0;
    queue_loc = 0;
    queue_end = 0;

    processes_size = DEFAULT_PROCESS_LIST_SIZE;
    threads_size = DEFAULT_THREAD_LIST_SIZE;
    last_pid = 0;
    last_tid = 0;

    currenttid = -1;

    sched_create_process_idle();
}

void set_thread_stack(uint32_t *esp_k) {
    if (currenttid == -1) { 
        //printf("setting stack of tid -1!\n");
        return;
    }
    threads[currenttid]->esp_k = esp_k;
}