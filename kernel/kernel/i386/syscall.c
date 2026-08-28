// System Call Handler (syscall.c)
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

#include <kernel/irq.h>
#include <kernel/vfs.h>
#include <kernel/pload.h>
#include <kernel/sched.h>
#include <kernel/kernel.h>
#include <kernel/liballoc.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

typedef int (*syscallfunc)(unsigned int[]);

syscallfunc syscalltable[256] = {0};

int syscall_exit(unsigned int args[]) {
    return sched_exit_process((int)args[0]);
}

int syscall_wait(unsigned int args[]) {
    return sched_wait_process((int)args[0]);
}

int syscall_read(unsigned int args[]) {
    switch ((int)args[0]) {
        case 0: // stdin
            int retval = currentps->terminal->read((char*)args[1], (size_t)args[2]);
            if (retval == 0) {
                sched_suspend_thread(1, currenttid);
            }
            return retval;
        case 1: // stdout
        case 2: // stderr
            return 0;
    }
    return vfs_read((int)args[0]-3, (void*)args[1], (size_t)args[2]);
}

int syscall_write(unsigned int args[]) {
    switch ((int)args[0]) {
        case 0: // stdin
            return 0;
        case 1: // stdout
        case 2: // stderr
            return currentps->terminal->write((const char*)args[1], (size_t)args[2]);
    }
    return vfs_write((int)args[0]-3, (const void*)args[1], (size_t)args[2]);
}

int syscall_open(unsigned int args[]) {
    int retval = vfs_open((char*)args[0], (int)args[1], (mode_t)args[2]);
    if (retval >= 0) {
        return retval+3;
    }
    return retval;
}

int syscall_close(unsigned int args[]) {
    return vfs_close((int)args[0]-3);
}

int syscall_exec(unsigned int args[]) {
    return pload_create_process_file((char*)args[0], (char**)args[1], 0);
}

int syscall_brk(unsigned int args[]) {
    return sched_setbrk((void*)args[0]);
}

int syscall_size(unsigned int args[]) {
    return vfs_size((int)args[0]-3);
}

int syscall_eret(unsigned int args[]) {
    if ((int)args[0] < 0) {
        // the process has requested to exit
        return sched_exit_process(-(int)args[0]);
    }
    // we don't need to do anything if the process has returned any other value
    return 0;
}

int syscall_ehndlr(unsigned int args[]) {
    currentps->procerrhandler = (perrhandle)args[0];
    return 0;
}

int syscall_getenv(unsigned int args[]) {
    char* eval = NULL;
    int elen = 0;
    for (int i = 0; i < currentps->environc; i++) {
        elen = strlen(currentps->environ[i]);
        int alen = strlen((char*)args[0]);
        if (alen >= elen) {
            continue; // impossible for this to be the one we're looking for
        }
        if (!strncmp(currentps->environ[i], (char*)args[0], alen)) {
            eval = currentps->environ[i];
            break;
        }
    }
    if (eval == NULL) {
        return 0;
    }
    for (int i = 0; i < elen; i++) {
        if (eval[i] == '=') {
            eval = &eval[i+1];
            elen -= i+1;
            break;
        }
    }
    memcpy((char*)args[1], eval, elen+1);
    return (int)args[1];
}

int syscall_setenv(unsigned int args[]) {
    int eindex = -1;
    int elen = 0;
    int alen = 0;
    for (int i = 0; i < currentps->environc; i++) {
        elen = strlen(currentps->environ[i]);
        alen = strlen((char*)args[0]);
        if (alen >= elen) {
            continue; // impossible for this to be the one we're looking for
        }
        if (!strncmp(currentps->environ[i], (char*)args[0], alen)) {
            eindex = i;
            break;
        }
    }
    if (eindex == -1) {
        eindex = currentps->environc;
        currentps->environc++;
    } else {
        kfree(currentps->environ[eindex]); // don't need the old one anymore
    }
    int fulllen = alen + 1 + strlen((char*)args[1]);
    currentps->environ[eindex] = kmalloc(fulllen+1);
    sprintf(currentps->environ[eindex], "%s=%s", (char*)args[0], (char*)args[1]);
    return 0;
}

int syscall_setup(unsigned int args[]) {
    // this is called on process start if the process needs arguments, and never again
    char *targv[currentps->argc];
    
    // push all arguments to the user stack
    for (int i = 0; i < currentps->argc; i++) {
        int arglen = strlen(currentps->argv[i])+1;
        if (arglen % 4) { arglen += 4 - (arglen % 4); } // keep the stack pointer aligned to 4 bytes
        ((struct regs*)args[6])->useresp -= arglen;
        memcpy((char*)(((struct regs*)args[6])->useresp), currentps->argv[i], arglen);
        targv[i] = (char*)(((struct regs*)args[6])->useresp);
    }

    // push argv to the user stack
    ((struct regs*)args[6])->useresp -= 4 * (currentps->argc + 1);
    char **userargv = (char**)(((struct regs*)args[6])->useresp);
    for (int i = 0; i < currentps->argc; i++) {
        userargv[i] = targv[i];
    }

    // push argc and the location of argv to the user stack
    ((struct regs*)args[6])->useresp -= 4;
    *(uint32_t*)(((struct regs*)args[6])->useresp) = (uint32_t)userargv;
    ((struct regs*)args[6])->useresp -= 4;
    *(uint32_t*)(((struct regs*)args[6])->useresp) = currentps->argc;
    return 0;
}

int syscall_getver(unsigned int args[]) {
    char *buf = (char*)args[0];
    strncpy(buf, K_VERSION, args[1]);
    return 0;
}

int syscall_chdir(unsigned int args[]) {
    if (vfs_find_directory((char*)args[0]) == NULL) { return -ENOENT; }
    int len = strlen((char*)args[0]) + 1;
    kfree(currentps->pwd);
    currentps->pwd = kmalloc(len);
    strncpy(currentps->pwd, (char*)args[0], len);
    return 0;
}

int syscall_list(unsigned int args[]) {
    struct vfs_directory* dir = vfs_find_directory((char*)args[0]);
    char* buf = (char*)args[1];
    size_t len = (size_t)args[2];
    if (dir == NULL) { return -ENOENT; }

    size_t byteswritten = 0;
    if (args[3]) {
        for (uint32_t i = 0; i < dir->directories_len; i++) {
            byteswritten += strlen(dir->directories[i]->name)+1;
            if (byteswritten > len) {
                break;
            }
            strncat(buf, dir->directories[i]->name, len-byteswritten);
            strncat(buf, "\n", len-byteswritten);
        }
        buf[byteswritten-1] = '\0';
        return dir->directories_len;
    } else {
        for (uint32_t i = 0; i < dir->files_len; i++) {
            byteswritten += strlen(dir->files[i]->name)+1;
            if (byteswritten > len) {
                break;
            }
            strncat(buf, dir->files[i]->name, len-byteswritten);
            strncat(buf, "\n", len-byteswritten);
        }
        buf[byteswritten-1] = '\0';
        return dir->files_len;
    }
}

int syscall_getcwd(unsigned int args[]) {
    strncpy((char*)args[0], currentps->pwd, (size_t)args[1]);
    return (int)args[0];
}

void syscall_handler(struct regs *r) {
    kmode = 0;
    if (syscalltable[r->eax]) {
        unsigned int args[] = {r->ecx, r->edx, r->ebx, r->esi, r->edi, r->ebp, (unsigned int)r};
        r->eax = syscalltable[r->eax](args);
    } else {
        r->eax = -ENOSYS;
    }
    kmode = 1;
}

void syscall_init(void) {
    irq_install_handler(0x60, &syscall_handler);

    syscalltable[1]  = &syscall_exit;
    syscalltable[2]  = &syscall_wait;
    syscalltable[3]  = &syscall_read;
    syscalltable[4]  = &syscall_write;
    syscalltable[5]  = &syscall_open;
    syscalltable[6]  = &syscall_close;
    syscalltable[7]  = &syscall_exec;
    syscalltable[8]  = &syscall_brk;
    syscalltable[9]  = &syscall_size;
    syscalltable[10] = &syscall_eret;
    syscalltable[11] = &syscall_ehndlr;
    syscalltable[12] = &syscall_getenv;
    syscalltable[13] = &syscall_setenv;
    syscalltable[14] = &syscall_setup;
    syscalltable[15] = &syscall_getver;
    syscalltable[16] = &syscall_chdir;
    syscalltable[17] = &syscall_list;
    syscalltable[18] = &syscall_getcwd;
}

