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
#include <string.h>

uint32_t pload_create_process_file(const char* path) {
    // we need a vfs driver
    return 0; // fail
}

// Create the special kernel process
uint32_t pload_create_process_k(uint32_t* cr3) {
    return sched_set_cr3(sched_create_process(0, 0, "glxykrnl"), cr3);
}

uint32_t pload_create_process(const char* data, const uint8_t privilege, const char* name) {
    struct elf32_header *header = (struct elf32_header*)data;
    // Check if the ELF file is compatible
    // This is probably a bit overkill but we don't care
    if (!strncmp(data, ELFMAG, 4)) { return 0; }
    if (header->e_machine != EM_386) { return 0; }
    if (header->e_type != ET_EXEC) { return 0; }
    if (header->e_version != EV_CURRENT) { return 0; }
    if (header->e_ident[EI_OSABI]) { return 0; }
    if (header->e_ident[EI_CLASS] != ELFCLASS32) { return 0; }
    if (header->e_ident[EI_DATA] != ELFDATA2LSB) { return 0; }
    uint32_t pid = sched_create_process(privilege, 3, name);
    for (int i = 0; i < header->e_phnum; i++) {
        struct elf32_progheader *pheader = (struct elf32_progheader*)&data[(header->e_phoff) + ((header->e_phentsize) * i)];
        if (pheader->p_type != PT_LOAD) { continue; }
    }
    return 0;
}