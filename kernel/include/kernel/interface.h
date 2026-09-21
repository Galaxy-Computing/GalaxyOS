#ifndef _KERNEL_INTERFACE_H
#define _KERNEL_INTERFACE_H

#include <stdint.h>
#include <kernel/sched.h>

#define INTERFACE_FLAG_PRIVATE 1

struct interface_message {
    int    pid;
    int    sender;
    void*  buf;
    size_t size;
};

struct interface {
    char*                 name;
    int                   creator_pid;
    struct interface_message** messages;
    uint32_t              messages_count;
    uint32_t              messages_size;
    int                   flags;
};

int interface_create(const char* name, int pid, int flags);
int interface_delete(const char* name, int pid);
int interface_send(const char* name, int senderpid, int recvpid, void* buf, size_t size);
int interface_recv(const char* name, int recvpid, void* buf, size_t size, size_t* size_out);
int interface_wait(const char* name, struct thread* thread);
int interface_has_msg(struct interface* interface, int recvpid);
void interface_init(void);

#endif