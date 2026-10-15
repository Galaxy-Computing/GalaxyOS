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
#include <kernel/pmm.h>
#include <kernel/interface.h>
#include <kernel/safecpy.h>
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

// TODO: double copy is not ideal for io performance
int syscall_read(unsigned int args[]) {
    void* newbuf = kmalloc(args[2]); // if the program calling this passed an insane size for this buffer this causes an OOM error, but not much we can do about that right now
    int retval = 0;
    switch ((int)args[0]) {
        case 0: // stdin
            retval = currentps->terminal->read(newbuf, (size_t)args[2]);
            if (retval == 0) {
                sched_suspend_thread(1, currenttid);
            }
            break;
        case 1: // stdout
        case 2: // stderr
            kfree(newbuf);
            return 0;
        default:
            retval = vfs_read((int)args[0]-3, newbuf, (size_t)args[2]);
            break;
            
    }
    if (safecpy((void*)args[1], newbuf, (size_t)args[2], 0) == -1) {
        kfree(newbuf);
        return -EFAULT;
    }
    kfree(newbuf);
    return retval;
}

// TODO: double copy is not ideal for io performance
int syscall_write(unsigned int args[]) {
    void* newbuf = kmalloc(args[2]); // if the program calling this passed an insane size for this buffer this causes an OOM error, but not much we can do about that right now
    if (safecpy(newbuf, (const void*)args[1], (size_t)args[2], 1) == -1) {
        kfree(newbuf);
        return -EFAULT;
    }
    int retval;
    switch ((int)args[0]) {
        case 0: // stdin
            kfree(newbuf);
            return 0;
        case 1: // stdout
        case 2: // stderr
            retval = currentps->terminal->write(newbuf, (size_t)args[2]);
            kfree(newbuf);
            return retval;
    }
    retval = vfs_write((int)args[0]-3, newbuf, (size_t)args[2]);
    kfree(newbuf);
    return retval;
}

int syscall_open(unsigned int args[]) {
    if (safelen((char*)args[0], 1) == -1) return -EFAULT;
    int retval = vfs_open((char*)args[0], (int)args[1], (mode_t)args[2]);
    if (retval >= 0) {
        return retval+3;
    }
    return retval;
}

int syscall_close(unsigned int args[]) {
    return vfs_close((int)args[0]-3);
}

// TODO: this is unsafe for the second argument
int syscall_exec(unsigned int args[]) {
    if (safelen((char*)args[0], 1) == -1) return -EFAULT;
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
        int alen = safelen((char*)args[0], 1);
        if (alen == -1) return -EFAULT;
        if (alen >= elen) {
            continue; // impossible for this to be the one we're looking for
        }
        if (!strncmp(currentps->environ[i], (char*)args[0], alen)) {
            eval = currentps->environ[i];
            break;
        }
    }
    if (eval == NULL) {
        return -ENOENT;
    }
    for (int i = 0; i < elen; i++) {
        if (eval[i] == '=') {
            eval = &eval[i+1];
            elen -= i+1;
            break;
        }
    }
    if (safecpy((char*)args[1], eval, elen+1, 0) == -1) return -EFAULT;
    return 0;
}

int syscall_setenv(unsigned int args[]) {
    if (safelen((char*)args[0], 1) == -1) return -EFAULT;
    if (safelen((char*)args[1], 1) == -1) return -EFAULT;

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
    int len = strlen(K_VERSION) + 1;
    if (len > args[1]) len = args[1];
    if (safecpy(buf, K_VERSION, len, 0) == -1) return -EFAULT;
    return 0;
}

int syscall_chdir(unsigned int args[]) {
    if (safelen((char*)args[0], 1) == -1) return -EFAULT;
    if (vfs_find_directory((char*)args[0]) == NULL) { return -ENOENT; }
    int len = strlen((char*)args[0]) + 1;
    kfree(currentps->pwd);
    currentps->pwd = kmalloc(len);
    strncpy(currentps->pwd, (char*)args[0], len);
    return 0;
}

// TODO: this is unsafe
int syscall_list(unsigned int args[]) {
    if (safelen((char*)args[0], 1) == -1) return -EFAULT;
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
    size_t len = strlen(currentps->pwd);
    if (len > (size_t)args[1]) len = (size_t)args[1];
    if (safecpy((char*)args[0], currentps->pwd, len, 0) == -1) return -EFAULT; 
    return (int)args[0];
}

int syscall_memusage(unsigned int args[]) {
    ((struct regs*)args[6])->ecx = pmm_used() * 4096;
    ((struct regs*)args[6])->edx = pmm_available() * 4096;
    ((struct regs*)args[6])->ebx = (pmm_used() + pmm_available()) * 4096;
    return (currentps->pages_used * 4096) + currentps->brk_offset;
}

int syscall_intfc(unsigned int args[]) {
    return interface_create((char*)args[0], currentps->pid, (int)args[1]);
}

int syscall_intfd(unsigned int args[]) {
    return interface_delete((char*)args[0], currentps->pid);
}

int syscall_intfs(unsigned int args[]) {
    return interface_send((char*)args[0], currentps->pid, (int)args[1], (void*)args[2], (size_t)args[3]);
}

int syscall_intfr(unsigned int args[]) {
    return interface_recv((char*)args[0], currentps->pid, (void*)args[1], (size_t)args[2], (size_t*)args[3]);
}

int syscall_intfw(unsigned int args[]) {
    return interface_wait((char*)args[0], threads[currenttid]);
}

int syscall_tfork(unsigned int args[]) {
    return sched_fork_thread(currenttid, (struct regs*)args[6]);
}

void syscall_handler(struct regs *r) {
    kmode = 0;
    if ((r->eax < 26) && (r->eax != 0)) {
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
    syscalltable[7]  = &syscall_exec; // unsafe
    syscalltable[8]  = &syscall_brk;
    syscalltable[9]  = &syscall_size;
    syscalltable[10] = &syscall_eret;
    syscalltable[11] = &syscall_ehndlr;
    syscalltable[12] = &syscall_getenv;
    syscalltable[13] = &syscall_setenv;
    syscalltable[14] = &syscall_setup;
    syscalltable[15] = &syscall_getver;
    syscalltable[16] = &syscall_chdir;
    syscalltable[17] = &syscall_list; // unsafe
    syscalltable[18] = &syscall_getcwd;
    syscalltable[19] = &syscall_memusage;
    syscalltable[20] = &syscall_intfc;
    syscalltable[21] = &syscall_intfd;
    syscalltable[22] = &syscall_intfs;
    syscalltable[23] = &syscall_intfr;
    syscalltable[24] = &syscall_intfw;
    syscalltable[25] = &syscall_tfork;
}

