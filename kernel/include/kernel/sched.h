#ifndef _KERNEL_SCHED_H
#define _KERNEL_SCHED_H

#include <stdint.h>
#include <kernel/vfs.h>
#include <kernel/term.h>

#define THREAD_STATE_RUNNING 0
#define THREAD_STATE_SUSPENDED 1
#define THREAD_STATE_WAITING 2
#define THREAD_STATE_STARTING 3
#define THREAD_STATE_EXITING 4
#define THREAD_STATE_DEAD 5

#define PROCESS_STATE_RUNNING 0
#define PROCESS_STATE_EXITING 1
#define PROCESS_STATE_STARTING 2
#define PROCESS_STATE_LOADING 3

typedef int (*perrhandle)(uint8_t);

struct ps_region {
    uintptr_t offset;
    uintptr_t size;
};

struct process {
    char       *name;
    char       **argv;
    int        argc;
    int        threadcount;
    int        *tids;      // this is a dynamic array
    int        tids_size;
    uint32_t   *cr3;       // pointer to the page tables
    uint32_t   *cr3_virt;  // where the process's page directory is mapped in kernel space
    int        pid;
    uint32_t   entrypoint;

    uintptr_t  brk;        // pointer to the program break in user memory
    uintptr_t  pgbrk;      // pointer to the end of the actual last page allocated
    uintptr_t  brk_offset; // amount of memory that is allocated for brk

    struct ps_region **psregions;     // memory regions allocated by the loader
    uint32_t         psregions_count;

    uint8_t    privilege_level;
    uint8_t    state;           // set to 1 if the process is exiting
    uint32_t   exitcode;
    int        waiting_threads; // this is the counter of other threads waiting for this process to exit

    uint8_t    procerr;    // if this is anything other than 0, an error has occurred and the process must handle it or die

    // if this is not null, it is called when procerr is non-zero and a thread belonging to this process is executed
    // if it is null, the process is killed
    // said thread could be any one of the process's threads, so it must be prepared for any given thread to be interrupted
    // if the handler returns a negative number, the process is killed and the positive version of that number is used as the exit code
    perrhandle procerrhandler;  

    // vfs stuff here
    struct vfs_file_open **openfiles;
    int                  openfiles_loc;
    int                  openfiles_size;

    struct terminal *terminal;
};

struct thread {
    uint32_t  *esp_k;
    uint32_t  *cr3;         // pointer to the page tables (this should match the one in process)
    uint32_t   privilege_level;

    // anything after this comment can be changed without interfering with the assembly code
    int       pid;
    int       wait;         // this is set to the irq that is being waited for if the thread is suspended, or the pid of the process it's waiting for
    int       stackpdi;     // index into cr3 of the location of the stack page directory for this thread
    uint32_t  *entrypoint;  // this is invalid unless state = THREAD_STATE_STARTING
    uint8_t   state;
} __attribute__((packed)); // this is because this will be accessed from asm

int sched_create_thread(int ownerpid, uint8_t noqueue, uint32_t entrypoint);
int sched_create_process(uint8_t privilege, const char* name);
void sched_init(void);
struct thread *sched_loop(void);
void sched_pick_next(void);
int sched_set_cr3(int pid, uint32_t* newcr3);
void sched_check_suspended_threads(uint8_t irq);
void sched_suspend_thread(uint8_t irq, int tid);
void sched_suspend_current_thread(uint8_t irq);
int sched_wait_process(int pid);
int sched_exit_process(int exitcode);
uintptr_t sched_setbrk(void* addr);
void sched_user_fault(int eno, uint32_t errorcode);

extern int currenttid;
extern struct process *currentps;

extern struct process **processes;
extern int processes_size;
extern int last_pid;

extern struct thread **threads;
extern int threads_size;
extern int last_tid;

#endif