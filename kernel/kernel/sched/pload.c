// Process Loader (pload.c)
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

#include <kernel/sched.h>
#include <kernel/pload.h>
#include <kernel/elf.h>
#include <kernel/vfs.h>
#include <kernel/liballoc.h>
#include <kernel/vmm.h>
#include <kernel/kernel.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

static inline void set_cr3(uint32_t phys_addr) {
    __asm__ __volatile__(
        "movl %0, %%cr3"
        :
        : "r" (phys_addr)
        : "memory"
    );
}

// returns non-zero if the file isn't compatible, 0 if it is
int elf_isvalid(const unsigned char* data) {
    struct elf32_header *header = (struct elf32_header*)data;
    // Check if the ELF file is compatible
    // This is probably a bit overkill but we don't care
    if (strncmp(data, ELFMAG, 4)) { return 1; }
    if (header->e_machine != EM_386) { return 2; }
    if (header->e_type != ET_EXEC) { return 3; }
    if (header->e_version != EV_CURRENT) { return 4; }
    if (header->e_ident[EI_OSABI]) { return 5; }
    if (header->e_ident[EI_CLASS] != ELFCLASS32) { return 6; }
    if (header->e_ident[EI_DATA] != ELFDATA2LSB) { return 7; }
    return 0;
}

int pload_create_process(const char* data, const uint8_t privilege, const char* name, char** args, int nodup) {
    int elfvalid = elf_isvalid(data);
    if (elfvalid) { 
        printf("invalid elf file: %i\n", elfvalid);
        return -1; 
    }
    struct elf32_header *header = (struct elf32_header*)data;

    int pid = sched_create_process(privilege, name);
    processes[pid]->state = 2;
    processes[pid]->entrypoint = (uint32_t)data; // we're borrowing this field to store the location of the elf data in memory
    if (args != NULL) {
        char* argptr = args[0];
        int argc = 0;
        while (argptr != NULL) {
            argc++;
            argptr = args[argc];
        }
        char** newargv = kmalloc(sizeof(char*) * argc);
        processes[pid]->argc = argc;
        for (int i = 0; i < argc; i++) {
            int arglen = strlen(args[i]) + 1;
            newargv[i] = kmalloc(arglen);
            memcpy(newargv[i], args[i], arglen);
        }
        processes[pid]->argv = newargv;
    }
    
    if (!nodup) {
        // duplicate environment variables and working directory
        processes[pid]->environ = kmalloc(sizeof(char**) * (currentps->environc + 1));
        processes[pid]->environc = currentps->environc;
        for (int i = 0; i < currentps->environc; i++) {
            int evarlen = strlen(currentps->environ[i]) + 1;
            processes[pid]->environ[i] = kmalloc(evarlen);
            memcpy(processes[pid]->environ[i], currentps->environ[i], evarlen);
        }
        processes[pid]->environ[processes[pid]->environc] = NULL;
        int pwdlen = strlen(currentps->pwd) + 1;
        processes[pid]->pwd = kmalloc(pwdlen);
        memcpy(processes[pid]->pwd, currentps->pwd, pwdlen);
    } else {
        // Set all values to null
        processes[pid]->environ = kcalloc(1, sizeof(char**));
        processes[pid]->environc = 0;
        processes[pid]->pwd = kcalloc(1, 1);
    }

    sched_create_thread(pid, 0, 0xFEFEFEFE); // entrypoint can be this magic number here since we will set that later
    return pid;
}

int pload_create_process_file(const char* path, char** args, int nodup) {
    int fd = vfs_open(path, O_RDONLY);
    if (fd < 0) { return fd; }
    struct stat *filestats = kmalloc(sizeof(struct stat));
    if (vfs_fstat(fd, filestats) == -1) {  
        printf("could not stat file\n");
        vfs_close(fd); 
        kfree(filestats);
        return -1;
    }
    char *data = kmalloc(filestats->st_size + 1);
    if (vfs_read(fd, data, filestats->st_size) == -1) { 
        printf("could not read file\n");
        vfs_close(fd); 
        kfree(filestats);
        kfree(data);
        return -1;
    }
    kfree(filestats);
    int retval = pload_create_process(data, 3, vfs_file_name(fd), args, nodup);
    
    vfs_close(fd);
    return retval;
}

int pload_load_process(int pid, int tid) {
    asm("cli");
    kmode = 0;
    processes[pid]->state = PROCESS_STATE_LOADING; // prevent the scheduler from calling us again
    // we need cr3 set to the process's page directory
    uintptr_t oldcr3;
    uintptr_t cr3 = (uintptr_t)processes[pid]->cr3;
    __asm__ __volatile__(
        "mov %%cr3, %0\n\t"
        : "=r" (oldcr3)
    );
    set_cr3(cr3);
    struct elf32_header *header = (struct elf32_header*)processes[pid]->entrypoint;
    char *data = (char*)processes[pid]->entrypoint;
    uintptr_t end_allocated_data = 0;
    //struct ps_region **psregions = kmalloc(sizeof(struct ps_region*) * header->e_phnum);
    //processes[pid]->psregions = psregions;
    for (int i = 0; i < header->e_phnum; i++) {
        struct elf32_progheader *pheader = (struct elf32_progheader*)&data[(header->e_phoff) + ((header->e_phentsize) * i)];
        if (pheader->p_type != PT_LOAD) { continue; }

        int pagecnt = pheader->p_memsz / 4096;
        if (pheader->p_memsz % 4096) { pagecnt += 1; }
        address_t pages[pagecnt];
                
        for (int x = 0; x < pagecnt; x++) {
            pages[x] = pmm_alloc_page();
        }
        uintptr_t loc = pheader->p_vaddr;
        if (vmm_map_pages_loc(pages, pagecnt, pheader->p_vaddr, 0x7) == NULL) {
            __asm__ (
                "mov %0, %%cr3\n\t"
                : : "r" (oldcr3)
            );
            return -1;
        } 
        memcpy((void*)pheader->p_vaddr, (void*)&data[pheader->p_offset], pheader->p_filesz);
        memset((void*)(pheader->p_vaddr + pheader->p_filesz), 0, pheader->p_memsz - pheader->p_filesz);
        if (end_allocated_data < pheader->p_memsz + pheader->p_vaddr) {
            end_allocated_data = pheader->p_memsz + pheader->p_vaddr;
        }
        struct ps_region *psr = kmalloc(sizeof(struct ps_region));
        if (psr == NULL) {
            printf("malloc psr returned 0!!!!\n");
            return 0;
        }
        psr->offset = pheader->p_vaddr;
        psr->size = pagecnt * 4096;
        processes[pid]->psregions[processes[pid]->psregions_count] = psr;
        processes[pid]->psregions_count++;
    }
    processes[pid]->brk = (end_allocated_data / 4096) * 4096;
    if (end_allocated_data % 4096) { processes[pid]->brk += 4096; }
    processes[pid]->pgbrk = processes[pid]->brk;
    processes[pid]->entrypoint = 0;
    processes[pid]->state = 0;
    *threads[tid]->entrypoint = header->e_entry;
    threads[tid]->state = THREAD_STATE_RUNNING;
    set_cr3(oldcr3);
    kmode = 1;
    asm("sti");
    return 0;
}