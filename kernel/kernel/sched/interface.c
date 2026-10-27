// Interface Manager (interface.c)
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

#include <kernel/interface.h>
#include <errno.h>
#include <kernel/sched.h>
#include <kernel/liballoc.h>
#include <kernel/kernel.h>
#include <kernel/safecpy.h>
#include <string.h>

struct interface **intfaces;
uint32_t intfaces_count;
uint32_t intfaces_size;

int interface_find(const char* name) {
    if (safelen(name, 1) == -1) return -EFAULT; // check if string is valid
    for (uint32_t i = 0; i < intfaces_count; i++) {
        if (!strcmp(name, intfaces[i]->name)) {
            return i;
        }
    }
    return -ENOENT;
}

int interface_create(const char* name, int pid, int flags) {
    int qid = interface_find(name);
    if (qid == -EFAULT) return -EFAULT;
    if (qid != -ENOENT) return -EEXIST;
    
    struct interface *newqueue = kmalloc(sizeof(struct interface));

    newqueue->creator_pid = pid;
    newqueue->name = strdup(name);
    newqueue->messages = kmalloc(sizeof(struct interface_message*) * 32);
    newqueue->messages_size = 32;
    newqueue->messages_count = 0;
    newqueue->flags = flags;

    if (intfaces_count >= intfaces_size) {
        intfaces = krealloc(intfaces, sizeof(struct interface*) * (intfaces_size + 32));
        intfaces_size += 32;
    }
    intfaces[intfaces_count] = newqueue;
    intfaces_count++;

    return 0;
}

int interface_delete(const char* name, int pid) {
    int qid = interface_find(name);
    if (qid < 0) return qid;
    if (intfaces[qid]->creator_pid != pid) return -EACCES;

    for (uint32_t i = 0; i < intfaces[qid]->messages_count; i++) {
        kfree(intfaces[qid]->messages[i]->buf);
        kfree(intfaces[qid]->messages[i]);
    }
    kfree(intfaces[qid]->messages);
    kfree(intfaces[qid]);

    for (uint32_t i = qid; i < intfaces_count-1; i++) {
        intfaces[i] = intfaces[i+1];
    }
    intfaces_count--;

    return 0;
}

int interface_send(const char* name, int senderpid, int recvpid, void* buf, size_t size) {
    int qid = interface_find(name);
    if (qid < 0) return qid;

    // only allow the creator to send messages to any pid, and only allow any pid to send a message to the creator
    if (intfaces[qid]->flags & INTERFACE_FLAG_PRIVATE) {
        if ((senderpid != intfaces[qid]->creator_pid) && (recvpid != 0)) {
            return -EACCES;
        }
    }

    if (recvpid == 0) {
        if (processes[intfaces[qid]->creator_pid] == NULL) {
            // tell the sender that the process it's trying to talk to is dead
            // this would usually mean that a system process has been killed, but it's userspace's responsibility to handle that, not ours (except for init)
            return -EOWNERDEAD; 
        }
    }

    if (intfaces[qid]->messages_count >= intfaces[qid]->messages_size) {
        intfaces[qid]->messages = krealloc(intfaces[qid]->messages, sizeof(struct interface_message*) * (intfaces[qid]->messages_size + 32));
        intfaces[qid]->messages_size += 32;
    }

    struct interface_message *newmsg = kmalloc(sizeof(struct interface_message));
    
    newmsg->pid = recvpid;
    newmsg->sender = senderpid;
    newmsg->buf = kmalloc(size);
    if (safecpy(newmsg->buf, buf, size, 1)) return -EFAULT;
    newmsg->size = size;

    intfaces[qid]->messages[intfaces[qid]->messages_count] = newmsg;
    intfaces[qid]->messages_count++;
    
    return 0;
}

int interface_recv(const char* name, int recvpid, void* buf, size_t size, size_t* size_out) {
    int qid = interface_find(name);
    if (qid < 0) return qid;

    int mid = -1;
    for (uint32_t i = 0; i < intfaces[qid]->messages_count; i++) {
        if (intfaces[qid]->messages[i]->pid == recvpid) {
            mid = i;
            break;
        } else if (intfaces[qid]->messages[i]->pid == 0) {
            if (intfaces[qid]->creator_pid == recvpid) {
                mid = i;
                break;
            }
        } else if (intfaces[qid]->messages[i]->pid == -1) {
            mid = i;
            break;
        }
    }

    if (mid == -1) return -EAGAIN;
    size_t truesize = intfaces[qid]->messages[mid]->size;
    if (size < truesize) truesize = size;

    if (safecpy(buf, intfaces[qid]->messages[mid]->buf, truesize, 0) == -1) return -EFAULT;

    int senderpid = intfaces[qid]->messages[mid]->sender;
    kfree(intfaces[qid]->messages[mid]->buf);
    kfree(intfaces[qid]->messages[mid]);

    for (uint32_t i = mid; i < intfaces[qid]->messages_count-1; i++) {
        intfaces[qid]->messages[i] = intfaces[qid]->messages[i+1];
    }
    intfaces[qid]->messages_count--;

    if (safecpy(size_out, &truesize, sizeof(size_t), 0) == -1) return -EFAULT;
    return senderpid;
}

int interface_has_msg(struct interface* interface, int recvpid) {
    for (uint32_t i = 0; i < interface->messages_count; i++) {
        if (interface->messages[i]->pid == recvpid) {
            return 1;
            break;
        } else if (interface->messages[i]->pid == 0) {
            if (interface->creator_pid == recvpid) {
                return 1;
                break;
            }
        } else if (interface->messages[i]->pid == -1) {
            return 1;
            break;
        }
    }
    return 0;
}

int interface_wait(const char* name, struct thread* thread) {
    int qid = interface_find(name);
    if (qid < 0) return qid;

    thread->state = THREAD_STATE_WAITING;
    thread->wait_msg = intfaces[qid];

    return 0;
}

void interface_init(void) {
    intfaces = kmalloc(sizeof(struct interface*) * 32);
    intfaces_size = 32;
    intfaces_count = 0;
}