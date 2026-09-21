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
#include <string.h>

struct interface **intfaces;
uint32_t intfaces_count;
uint32_t intfaces_size;

int interface_find(const char* name) {
    for (int i = 0; i < intfaces_count; i++) {
        if (!strcmp(name, intfaces[i]->name)) {
            return i;
        }
    }
    return -1;
}

int interface_create(const char* name, int pid, int flags) {
    int qid = interface_find(name);
    if (qid != -1) return -EEXIST;
    
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
    if (qid == -1) return -ENOENT;
    if (intfaces[qid]->creator_pid != pid) return -EACCES;

    for (int i = 0; i < intfaces[qid]->messages_count; i++) {
        kfree(intfaces[qid]->messages[i]->buf);
        kfree(intfaces[qid]->messages[i]);
    }
    kfree(intfaces[qid]->messages);
    kfree(intfaces[qid]);

    for (int i = qid; i < intfaces_count-1; i++) {
        intfaces[i] = intfaces[i+1];
    }
    intfaces_count--;

    return 0;
}

int interface_send(const char* name, int senderpid, int recvpid, void* buf, size_t size) {
    int qid = interface_find(name);
    if (qid == -1) return -ENOENT;

    // only allow the creator to send messages to any pid, and only allow any pid to send a message to the creator
    if (intfaces[qid]->flags & INTERFACE_FLAG_PRIVATE) {
        if ((senderpid != intfaces[qid]->creator_pid) && (recvpid != 0)) {
            return -EACCES;
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
    memcpy(newmsg->buf, buf, size);
    newmsg->size = size;

    intfaces[qid]->messages[intfaces[qid]->messages_count] = newmsg;
    intfaces[qid]->messages_count++;
    
    return 0;
}

int interface_recv(const char* name, int recvpid, void* buf, size_t size, size_t* size_out) {
    int qid = interface_find(name);
    if (qid == -1) return -ENOENT;

    int mid = -1;
    for (int i = 0; i < intfaces[qid]->messages_count; i++) {
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

    memcpy(buf, intfaces[qid]->messages[mid]->buf, truesize);

    int senderpid = intfaces[qid]->messages[mid]->sender;
    kfree(intfaces[qid]->messages[mid]->buf);
    kfree(intfaces[qid]->messages[mid]);

    for (int i = mid; i < intfaces[qid]->messages_count-1; i++) {
        intfaces[qid]->messages[i] = intfaces[qid]->messages[i+1];
    }
    intfaces[qid]->messages_count--;

    *size_out = truesize;
    return senderpid;
}

int interface_has_msg(struct interface* interface, int recvpid) {
    for (int i = 0; i < interface->messages_count; i++) {
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
    if (qid == -1) return -ENOENT;

    thread->state = THREAD_STATE_WAITING;
    thread->wait_msg = intfaces[qid];

    return 0;
}

void interface_init(void) {
    intfaces = kmalloc(sizeof(struct interface*) * 32);
    intfaces_size = 32;
    intfaces_count = 0;
}