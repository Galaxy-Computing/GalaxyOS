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
#include <kernel/vmm.h>
#include <kernel/power.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

typedef int (*syscallfunc)(unsigned int[]);

syscallfunc syscalltable[256] = {0};

// max bytes per transfer (kmalloc panics on OOM)
#define MAX_SYSCALL_IO_SIZE (1024 * 1024)

// userspace string limits
#define MAX_PATH_LEN           4096
#define MAX_ENV_NAME_SIZE      4096
#define MAX_ENV_VALUE_SIZE     65536
#define MAX_ENVIRON_VARS       4096
#define MAX_INTERFACE_NAME_LEN 1024

// exec argument limits
#define MAX_EXEC_ARGS          1024
#define MAX_EXEC_ARG_SIZE      65536
#define MAX_EXEC_ARGS_TOTAL    (1024 * 1024)

// process loader/fork stack region
#define USER_STACK_REGION_SIZE 0x400000

static int user_buffer_ok(uintptr_t ptr, size_t size) {
    if (size == 0) return 1;
    if (ptr >= KERNEL_START) return 0;
    return size <= (size_t)(KERNEL_START - ptr);
}

// copy a userspace string to a new kernel buffer, NULL and *err set on error
static char *kstrdup_user(const void *uptr, size_t maxlen, int *err) {
    *err = 0;
    ssize_t len = safelen_max_err((const char*)uptr, maxlen, err);
    if (len < 0) return NULL;

    char *buf = kmalloc((size_t)len + 1);
    if (safecpy_from_user(buf, uptr, (size_t)len + 1) == -1) {
        kfree(buf);
        *err = -EFAULT;
        return NULL;
    }
    return buf;
}

// copy a userspace argv vector and its strings, NULL and *err set on error
static char **kargv_user(const void *uargv, int *err) {
    *err = 0;
    if (uargv == NULL) return NULL;

    size_t cap = 8;
    char **argv = kmalloc(sizeof(char*) * cap);
    if (argv == NULL) {
        *err = -ENOMEM;
        return NULL;
    }

    int argc = 0;
    size_t total = 0;
    for (;;) {
        char *uarg = NULL;
        if (safecpy_from_user(&uarg, (const char*)uargv + (size_t)argc * sizeof(char*), sizeof(uarg)) == -1) {
            *err = -EFAULT;
            break;
        }
        if (uarg == NULL) {
            argv[argc] = NULL;
            return argv;
        }
        if (argc >= MAX_EXEC_ARGS) {
            *err = -E2BIG;
            break;
        }

        int lerr = 0;
        ssize_t len = safelen_max_err(uarg, MAX_EXEC_ARG_SIZE, &lerr);
        if (len < 0) {
            *err = (lerr == -ENAMETOOLONG) ? -E2BIG : lerr;
            break;
        }
        if ((size_t)len + 1 > MAX_EXEC_ARGS_TOTAL - total) {
            *err = -E2BIG;
            break;
        }
        total += (size_t)len + 1;

        if ((size_t)argc + 1 >= cap) {
            size_t newcap = cap * 2;
            if (newcap > MAX_EXEC_ARGS + 1) newcap = MAX_EXEC_ARGS + 1;
            char **newargv = krealloc(argv, sizeof(char*) * newcap);
            if (newargv == NULL) {
                *err = -ENOMEM;
                break;
            }
            argv = newargv;
            cap = newcap;
        }

        char *copy = kmalloc((size_t)len + 1);
        if (copy == NULL) {
            *err = -ENOMEM;
            break;
        }
        if (safecpy_from_user(copy, uarg, (size_t)len + 1) == -1) {
            kfree(copy);
            *err = -EFAULT;
            break;
        }
        argv[argc++] = copy;
    }

    for (int i = 0; i < argc; i++) kfree(argv[i]);
    kfree(argv);
    return NULL;
}

static void kargv_free(char **argv) {
    if (argv == NULL) return;
    for (int i = 0; argv[i] != NULL; i++) kfree(argv[i]);
    kfree(argv);
}

int syscall_exit(unsigned int args[]) {
    return sched_exit_process((int)args[0]);
}

int syscall_wait(unsigned int args[]) {
    return sched_wait_process((int)args[0]);
}

// TODO: double copy is not ideal for io performance
int syscall_read(unsigned int args[]) {
    size_t count = (size_t)args[2];
    if (count == 0) return 0;
    if (count > MAX_SYSCALL_IO_SIZE) count = MAX_SYSCALL_IO_SIZE;
    if (!user_buffer_ok((uintptr_t)args[1], count)) return -EFAULT;

    void* newbuf = kmalloc(count);
    int retval = 0;
    switch ((int)args[0]) {
        case 0: // stdin
            // don't block for a keypress if the destination isn't writable
            if (!vmm_user_range_writable((address_t)args[1], count)) {
                kfree(newbuf);
                return -EFAULT;
            }
            retval = currentps->terminal->read(newbuf, count);
            if (retval == 0) {
                sched_suspend_thread(1, currenttid);
            }
            break;
        case 1: // stdout
        case 2: // stderr
            kfree(newbuf);
            return 0;
        default:
            retval = vfs_read((int)args[0]-3, newbuf, count);
            break;
            
    }
    // only copy what the driver actually produced
    size_t copycount = 0;
    if (retval > 0) {
        copycount = (size_t)retval;
        if (copycount > count) copycount = count;
    }
    if (copycount && safecpy_to_user((void*)args[1], newbuf, copycount) == -1) {
        kfree(newbuf);
        return -EFAULT;
    }
    kfree(newbuf);
    return retval;
}

// TODO: double copy is not ideal for io performance
int syscall_write(unsigned int args[]) {
    size_t count = (size_t)args[2];
    if (count == 0) return 0;
    if (count > MAX_SYSCALL_IO_SIZE) count = MAX_SYSCALL_IO_SIZE;
    if (!user_buffer_ok((uintptr_t)args[1], count)) return -EFAULT;

    void* newbuf = kmalloc(count);
    if (safecpy_from_user(newbuf, (const void*)args[1], count) == -1) {
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
            retval = currentps->terminal->write(newbuf, count);
            kfree(newbuf);
            return retval;
    }
    retval = vfs_write((int)args[0]-3, newbuf, count);
    kfree(newbuf);
    return retval;
}

int syscall_open(unsigned int args[]) {
    int err;
    char *path = kstrdup_user((void*)args[0], MAX_PATH_LEN, &err);
    if (path == NULL) return err;
    int retval = vfs_open(path, (int)args[1], (mode_t)args[2]);
    kfree(path);
    if (retval >= 0) {
        return retval+3;
    }
    return retval;
}

int syscall_close(unsigned int args[]) {
    int fd = (int)args[0];
    if (fd < 3) return -EBADF; // stdin/stdout/stderr can't be closed
    return vfs_close(fd - 3);
}

int syscall_exec(unsigned int args[]) {
    int err;
    char *path = kstrdup_user((void*)args[0], MAX_PATH_LEN, &err);
    if (path == NULL) return err;

    char **argv = kargv_user((void*)args[1], &err);
    if (argv == NULL && err != 0) {
        kfree(path);
        return err;
    }

    int retval = pload_create_process_file(path, argv, 0);
    kargv_free(argv);
    kfree(path);
    return retval;
}

int syscall_brk(unsigned int args[]) {
    return sched_setbrk((void*)args[0]);
}

int syscall_size(unsigned int args[]) {
    int fd = (int)args[0];
    if (fd < 3) return -EBADF;
    return vfs_size(fd - 3);
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
    if ((args[0] != 0) && (args[0] >= KERNEL_START)) return -EFAULT;
    currentps->procerrhandler = (perrhandle)args[0];
    return 0;
}

int syscall_getenv(unsigned int args[]) {
    int err;
    char *name = kstrdup_user((void*)args[0], MAX_ENV_NAME_SIZE, &err);
    if (name == NULL) return err;
    size_t namelen = strlen(name);

    char* eval = NULL;
    for (int i = 0; i < currentps->environc; i++) {
        char* var = currentps->environ[i];
        if (var == NULL) continue;
        if (strlen(var) > namelen && !strncmp(var, name, namelen) && var[namelen] == '=') {
            eval = &var[namelen + 1];
            break;
        }
    }
    kfree(name);
    if (eval == NULL) {
        return -ENOENT;
    }

    size_t size = (size_t)args[2];
    if (size == 0) return 0;

    size_t vallen = strlen(eval);
    size_t copylen = vallen + 1;
    if (copylen > size) {
        // truncate, but keep it terminated
        size_t n = size - 1;
        if (n > 0 && safecpy_to_user((char*)args[1], eval, n) == -1) return -EFAULT;
        char nul = '\0';
        if (safecpy_to_user((char*)args[1] + n, &nul, 1) == -1) return -EFAULT;
        return 0;
    }
    if (safecpy_to_user((char*)args[1], eval, copylen) == -1) return -EFAULT;
    return 0;
}

int syscall_setenv(unsigned int args[]) {
    int err;
    char *name = kstrdup_user((void*)args[0], MAX_ENV_NAME_SIZE, &err);
    if (name == NULL) return err;
    size_t namelen = strlen(name);
    if (namelen == 0) {
        kfree(name);
        return -EINVAL;
    }

    ssize_t vallen = safelen_max_err((const char*)args[1], MAX_ENV_VALUE_SIZE + 1, &err);
    if (vallen == -1) {
        kfree(name);
        // value cap, not a name limit
        return (err == -ENAMETOOLONG) ? -E2BIG : err;
    }

    int eindex = -1;
    for (int i = 0; i < currentps->environc; i++) {
        char* var = currentps->environ[i];
        if (var == NULL) continue;
        if (strlen(var) > namelen && !strncmp(var, name, namelen) && var[namelen] == '=') {
            eindex = i;
            break;
        }
    }

    size_t fulllen = namelen + 1 + (size_t)vallen + 1;
    char* newvar = kmalloc(fulllen);
    memcpy(newvar, name, namelen);
    newvar[namelen] = '=';
    if (safecpy_from_user(&newvar[namelen + 1], (const void*)args[1], (size_t)vallen) == -1) {
        kfree(newvar);
        kfree(name);
        return -EFAULT;
    }
    newvar[namelen + 1 + vallen] = '\0';
    kfree(name);

    if (eindex == -1) {
        // cap total variables
        if (currentps->environc >= MAX_ENVIRON_VARS) {
            kfree(newvar);
            return -ENOSPC;
        }
        // grow environ before adding a var, leaving room for the trailing NULL
        if ((currentps->environc + 1) >= currentps->environ_size) {
            int newsize = currentps->environ_size ? (currentps->environ_size * 2) : 4;
            if (newsize <= currentps->environc) newsize = currentps->environc + 4;
            currentps->environ = krealloc(currentps->environ, sizeof(char*) * newsize);
            currentps->environ_size = newsize;
        }
        eindex = currentps->environc;
        currentps->environc++;
    } else {
        kfree(currentps->environ[eindex]); // don't need the old one anymore
    }
    currentps->environ[eindex] = newvar;
    if (currentps->environc < currentps->environ_size) {
        currentps->environ[currentps->environc] = NULL;
    }
    return 0;
}

int syscall_setup(unsigned int args[]) {
    // this is called on process start if the process needs arguments, and never again
    struct regs *r = (struct regs*)args[6];
    uintptr_t stackbase = (uintptr_t)threads[currenttid]->stackpdi * USER_STACK_REGION_SIZE;
    uintptr_t stackend = stackbase + USER_STACK_REGION_SIZE;
    int argc = currentps->argc;

    if (argc < 0) return -EINVAL;
    if (r->useresp < stackbase || r->useresp > stackend) return -EFAULT;

    char **targv = NULL;
    if (argc > 0) {
        targv = kmalloc(sizeof(char*) * argc);
    }

    // push all arguments to the user stack
    for (int i = 0; i < argc; i++) {
        size_t arglen = strlen(currentps->argv[i]) + 1;
        if (arglen % 4) { arglen += 4 - (arglen % 4); } // keep the stack pointer aligned to 4 bytes
        if (arglen > r->useresp - stackbase) goto setup_fail;
        r->useresp -= arglen;
        memcpy((char*)r->useresp, currentps->argv[i], arglen);
        targv[i] = (char*)r->useresp;
    }

    // push argv to the user stack
    size_t argvsize = sizeof(char*) * (argc + 1);
    if (argvsize > r->useresp - stackbase) goto setup_fail;
    r->useresp -= argvsize;
    char **userargv = (char**)r->useresp;
    for (int i = 0; i < argc; i++) {
        userargv[i] = targv[i];
    }
    userargv[argc] = NULL;

    // push argc and the location of argv to the user stack
    if (sizeof(uint32_t) * 2 > r->useresp - stackbase) goto setup_fail;
    r->useresp -= 4;
    *(uint32_t*)r->useresp = (uint32_t)userargv;
    r->useresp -= 4;
    *(uint32_t*)r->useresp = (uint32_t)argc;

    if (targv != NULL) kfree(targv);
    return 0;

setup_fail:
    if (targv != NULL) kfree(targv);
    return -E2BIG;
}

int syscall_getver(unsigned int args[]) {
    char *buf = (char*)args[0];
    size_t size = (size_t)args[1];
    size_t len = strlen(K_VERSION) + 1;
    if (len > size) len = size;
    if (safecpy_to_user(buf, K_VERSION, len) == -1) return -EFAULT;
    return 0;
}

int syscall_chdir(unsigned int args[]) {
    int err;
    char *path = kstrdup_user((void*)args[0], MAX_PATH_LEN, &err);
    if (path == NULL) return err;

    struct vfs_directory *newdir = vfs_find_directory(path);
    kfree(path);
    if (newdir == NULL) { return -ENOENT; }

    char *newpwd = kmalloc(MAX_PATH_LEN);
    if (vfs_get_path_directory(newdir, newpwd, MAX_PATH_LEN) == (size_t)-1) {
        kfree(newpwd);
        return -ENAMETOOLONG;
    }
    kfree(currentps->pwd);
    currentps->pwd = newpwd;
    return 0;
}

int syscall_list(unsigned int args[]) {
    int err;
    char *path = kstrdup_user((void*)args[0], MAX_PATH_LEN, &err);
    if (path == NULL) return err;
    struct vfs_directory* dir = vfs_find_directory(path);
    kfree(path);
    char* userbuf = (char*)args[1];
    size_t len = (size_t)args[2];
    int dirs = args[3] != 0;
    if (dir == NULL) { return -ENOENT; }

    uint32_t count = dirs ? dir->directories_len : dir->files_len;
    if (len == 0) { return (int)count; } // no room to write anything
    if (!safecpy_check_user(userbuf, len)) { return -EFAULT; }

    // build in kernel memory first
    size_t needed = 0;
    for (uint32_t i = 0; i < count; i++) {
        const char* name = dirs ? dir->directories[i]->name : dir->files[i]->name;
        size_t namelen = strlen(name);
        if (namelen + 1 > len - needed) {
            needed = len;
            break;
        }
        needed += namelen + 1;
    }
    if (needed == 0) { needed = 1; } // room for the terminating null byte

    char* buf = kmalloc(needed);
    if (buf == NULL) { return -ENOMEM; }

    size_t byteswritten = 0;
    for (uint32_t i = 0; i < count; i++) {
        const char* name = dirs ? dir->directories[i]->name : dir->files[i]->name;
        size_t namelen = strlen(name);
        if (namelen + 1 > needed - byteswritten) {
            break; // no room left for this entry
        }
        memcpy(&buf[byteswritten], name, namelen);
        byteswritten += namelen;
        buf[byteswritten] = '\n';
        byteswritten++;
    }
    // turn the last newline into a terminator
    if (byteswritten > 0) { byteswritten--; }
    buf[byteswritten] = '\0';

    int ret = safecpy_to_user(userbuf, buf, byteswritten + 1);
    kfree(buf);
    if (ret == -1) { return -EFAULT; }
    return (int)count;
}

int syscall_getcwd(unsigned int args[]) {
    if (currentps->pwd == NULL) return -ENOENT;
    size_t len = strlen(currentps->pwd) + 1;
    if (len > (size_t)args[1]) len = (size_t)args[1];
    if (safecpy_to_user((char*)args[0], currentps->pwd, len) == -1) return -EFAULT;
    return (int)args[0];
}

int syscall_memusage(unsigned int args[]) {
    ((struct regs*)args[6])->ecx = pmm_used() * 4096;
    ((struct regs*)args[6])->edx = pmm_available() * 4096;
    ((struct regs*)args[6])->ebx = (pmm_used() + pmm_available()) * 4096;
    return (currentps->pages_used * 4096) + currentps->brk_offset;
}

// copy the name first, another thread could change it
static char *interface_name_user(const void *uptr, int *err) {
    return kstrdup_user(uptr, MAX_INTERFACE_NAME_LEN, err);
}

int syscall_intfc(unsigned int args[]) {
    int err;
    char *name = interface_name_user((void*)args[0], &err);
    if (name == NULL) return err;
    int retval = interface_create(name, currentps->pid, (int)args[1]);
    kfree(name);
    return retval;
}

int syscall_intfd(unsigned int args[]) {
    int err;
    char *name = interface_name_user((void*)args[0], &err);
    if (name == NULL) return err;
    int retval = interface_delete(name, currentps->pid);
    kfree(name);
    return retval;
}

int syscall_intfs(unsigned int args[]) {
    int err;
    char *name = interface_name_user((void*)args[0], &err);
    if (name == NULL) return err;
    int retval = interface_send(name, currentps->pid, (int)args[1], (void*)args[2], (size_t)args[3]);
    kfree(name);
    return retval;
}

int syscall_intfr(unsigned int args[]) {
    int err;
    char *name = interface_name_user((void*)args[0], &err);
    if (name == NULL) return err;
    int retval = interface_recv(name, currentps->pid, (void*)args[1], (size_t)args[2], (size_t*)args[3]);
    kfree(name);
    return retval;
}

int syscall_intfw(unsigned int args[]) {
    int err;
    char *name = interface_name_user((void*)args[0], &err);
    if (name == NULL) return err;
    int retval = interface_wait(name, threads[currenttid]);
    kfree(name);
    return retval;
}

int syscall_tfork(unsigned int args[]) {
    return sched_fork_thread(currenttid, (struct regs*)args[6]);
}

int syscall_spstate(unsigned int args[]) {
    switch ((int)args[0]) {
        case 0:
            return 0; // if we're here we're already running, so do nothing
        case 1:
            return -ENOSYS; // TODO: implement shutdown
        case 2:
            // only init may reboot
            if (currentps->pid != 1) return -EACCES;
            // if we are here, init should have cleaned up everything in userspace
            reboot();
            return 0;
        default:
            return -EINVAL; // we don't recognize this state
    }
}

int syscall_kill(unsigned int args[]) {
    return sched_kill_process((int)args[0]);
}

int syscall_pscount(unsigned int args[]) {
    return processes_count;
}

int syscall_pslist(unsigned int args[]) {
    size_t count = 0;
    for (int i = 0; i < last_pid; i++) {
        if (!((count) < (size_t)args[1])) {
            return (int)count;
        }
        if (processes[i] != NULL) {
            if (safecpy_to_user(&(((int*)args[0])[count]), &i, sizeof(int))) return -EFAULT;
            count++;
        }
    }
    return (int)count;
}

int syscall_texit(unsigned int args[]) {
    sched_exit_thread();
    return 0;
}

void syscall_handler(struct regs *r) {
    kmode = 0;
    if ((r->eax < 31) && (r->eax != 0)) { // change that number when adding a new syscall
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
    syscalltable[19] = &syscall_memusage;
    syscalltable[20] = &syscall_intfc;
    syscalltable[21] = &syscall_intfd;
    syscalltable[22] = &syscall_intfs;
    syscalltable[23] = &syscall_intfr;
    syscalltable[24] = &syscall_intfw;
    syscalltable[25] = &syscall_tfork;
    syscalltable[26] = &syscall_spstate;
    syscalltable[27] = &syscall_kill;
    syscalltable[28] = &syscall_pscount;
    syscalltable[29] = &syscall_pslist;
    syscalltable[30] = &syscall_texit;
}

