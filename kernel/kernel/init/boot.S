# Boot Assembly (boot.S)
# Copyright (C) 2025 Skye310 (Galaxy Computing)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a multiboot header that marks the program as a kernel.
.section .multiboot.data, "aw"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Allocate the initial stack.
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

# Preallocate pages used for paging. Don't hard-code addresses and assume they
# are available, as the bootloader might have loaded its multiboot structures or
# modules there. This lets the bootloader know it must avoid the addresses.
.section .bss, "aw", @nobits
	.align 4096
page_table:
	.skip 3145728
boot_page_table1:
	.skip 4096
boot_page_table2:
	.skip 4096
boot_page_table3:
	.skip 1036288 # 1mb - 4096*3
boot_page_directory:
	.skip 4096

# Further page tables may be required if the kernel grows beyond 3 MiB.

# The kernel entry point.
.section .multiboot.text, "a"
.global _start
.type _start, @function
_start:

	# Initialize the page tables to 0.
	mov $4194304, %ecx
	movl $(page_table - 0xC0000000), %edi
zero_loop:
    stosb
    loop zero_loop

	# Physical address of boot_page_table1.
	# TODO: I recall seeing some assembly that used a macro to do the
	#       conversions to and from physical. Maybe this should be done in this
	#       code as well?
	movl $(boot_page_table1 - 0xC0000000), %edi
	# First address to map is address 0.
	# TODO: Start at the first kernel page instead. Alternatively map the first
	#       1 MiB as it can be generally useful, and there's no need to
	#       specially map the VGA buffer.
	movl $0, %esi
	# Map 3072 pages.
	movl $3072, %ecx

1:
	#cmpl $_kernel_start, %esi
	#jl 2f
	#cmpl $(_kernel_end - 0xC0000000), %esi
	#jge 3f

	# Map physical address as "present, writable". Note that this maps
	# .text and .rodata as writable. Mind security and map them as non-writable.
	movl %esi, %edx
	orl $0x003, %edx
	movl %edx, (%edi)

2:
	# Size of page is 4096 bytes.
	addl $4096, %esi
	# Size of entries in boot_page_table1 is 4 bytes.
	addl $4, %edi
	# Loop to the next entry if we haven't finished.
	loop 1b

3:
	# Map VGA video memory to 0xC0BFF000 as "present, writable".
	movl $(0x000B8000 | 0x003), boot_page_table3 - 0xC0000000 + 1023 * 4

	# The page table is used at both page directory entry 0 (virtually from 0x0
	# to 0xBFFFFF) (thus identity mapping the kernel) and page directory entry
	# 768 (virtually from 0xC0000000 to 0xC0BFFFFF) (thus mapping it in the
	# higher half). The kernel is identity mapped because enabling paging does
	# not change the next instruction, which continues to be physical. The CPU
	# would instead page fault if there was no identity mapping.

	# Map the page tables to both virtual addresses 0x00000000 and 0xC0000000.
	movl $(boot_page_table1 - 0xC0000000 + 0x007), boot_page_directory - 0xC0000000 + 0
	movl $(boot_page_table1 - 0xC0000000 + 0x007), boot_page_directory - 0xC0000000 + 768 * 4
	movl $(boot_page_table2 - 0xC0000000 + 0x007), boot_page_directory - 0xC0000000 + 4
	movl $(boot_page_table2 - 0xC0000000 + 0x007), boot_page_directory - 0xC0000000 + 769 * 4
	movl $(boot_page_table3 - 0xC0000000 + 0x007), boot_page_directory - 0xC0000000 + 8
	movl $(boot_page_table3 - 0xC0000000 + 0x007), boot_page_directory - 0xC0000000 + 770 * 4

	# Map the page tables and directory to 0xFFC00000.
	movl $(boot_page_directory - 0xC0000000 + 0x003), boot_page_directory - 0xC0000000 + 1023 * 4

	# Set cr3 to the address of the boot_page_directory.
	movl $(boot_page_directory - 0xC0000000), %ecx
	movl %ecx, %cr3

	# Enable paging and the write-protect bit.
	movl %cr0, %ecx
	orl $0x80010000, %ecx
	movl %ecx, %cr0

	# Jump to higher half with an absolute jump. 
	lea 4f, %ecx
	jmp *%ecx

.section .text

4:
	# At this point, paging is fully set up and enabled.

	# Unmap the identity mapping as it is now unnecessary. 
	movl $0, boot_page_directory + 0
	movl $0, boot_page_directory + 4
	movl $0, boot_page_directory + 8

	# Reload crc3 to force a TLB flush so the changes to take effect.
	movl %cr3, %ecx
	movl %ecx, %cr3

	# Set up the stack.
	mov $stack_top, %esp

	# Push the multiboot arguments onto the stack.
	push $(page_table - 0xC0000000)
	push %eax # Magic (should be 0x2BADBOO2)
	push %ebx # Address of the multiboot info

	call _init

	# Enter the high-level kernel.
	call kernel_main

.global halt
halt:	
	# Infinite loop if the system has nothing more to do.
	cli
	# call _fini
	hlt
	jmp 1b

.global load_gdt
load_gdt:
    mov 4(%esp), %edx    # EDX is 1st argument - GDT record pointer
    mov $0x10, %eax    # EAX is 2nd argument - Data Selector   
    lgdt (%edx)          # Load GDT with GDT record pointer passed as 1st argument
    mov %eax, %ds        # Reload all the data descriptors with Data selector (2nd arg)
    mov %eax, %es
    mov %eax, %gs
    mov %eax, %fs
    mov %eax, %ss
    ljmp $0x08, $setcs       # Do the FAR JMP to next instruction to set CS with Code selector,
                        #    and set the EIP (instruction pointer) to offset of setcs
setcs:
    ret

.global idt_load
.extern idtp
idt_load:
    lidt (idtp)
    ret

.global init_fpu
init_fpu:
	mov %cr0, %edx
	and $0xFFFFFFF3, %edx # clear EM and TS
	mov %edx, %cr0
	fninit
	fnstsw (testword)
	cmpw $0, (testword)
	jne nofpu
	jmp hasfpu

nofpu:
	mov $1, %eax # failure
	ret

hasfpu:
	fninit
	mov $0, %eax # success
	ret

testword:
.word 0x55AA

.global isr_0
.global isr_1
.global isr_2
.global isr_3
.global isr_4
.global isr_5
.global isr_6
.global isr_7
.global isr_8
.global isr_9
.global isr_10
.global isr_11
.global isr_12
.global isr_13
.global isr_14
.global isr_16
.global isr_17
.global isr_18
.global isr_19
.global isr_20
.global isr_21
.global isr_28
.global isr_29
.global isr_30

.extern exception_handle

isr_0:
	cli
	push $0
	push $0
	jmp isr_common_stub

isr_1:
	cli
	push $0
	push $1
	jmp isr_common_stub

isr_2:
	cli
	push $0
	push $2
	jmp isr_common_stub

isr_3:
	cli
	push $0
	push $3
	jmp isr_common_stub

isr_4:
	cli
	push $0
	push $4
	jmp isr_common_stub

isr_5:
	cli
	push $0
	push $5
	jmp isr_common_stub

isr_6:
	cli
	push $0
	push $6
	jmp isr_common_stub

isr_7:
	cli
	push $0
	push $7
	jmp isr_common_stub

isr_8:
	cli
	push $8
	jmp isr_common_stub

isr_9:
	cli
	push $0
	push $9
	jmp isr_common_stub

isr_10:
	cli
	push $10
	jmp isr_common_stub

isr_11:
	cli
	push $11
	jmp isr_common_stub

isr_12:
	cli
	push $12
	jmp isr_common_stub

isr_13:
	cli
	push $13
	jmp isr_common_stub

isr_14:
	cli
	push $14
	jmp isr_common_stub

isr_16:
	cli
	push $0
	push $16
	jmp isr_common_stub

isr_17:
	cli
	push $17
	jmp isr_common_stub

isr_18:
	cli
	push $0
	push $18
	jmp isr_common_stub

isr_19:
	cli
	push $0
	push $19
	jmp isr_common_stub

isr_20:
	cli
	push $0
	push $20
	jmp isr_common_stub

isr_21:
	cli
	push $21
	jmp isr_common_stub

isr_28:
	cli
	push $0
	push $28
	jmp isr_common_stub

isr_29:
	cli
	push $0
	push $29
	jmp isr_common_stub

isr_30:
	cli
	push $30
	jmp isr_common_stub

isr_common_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax   # Load the Kernel Data Segment descriptor!
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %esp, %eax   # Push us the stack
    push %eax
    mov $exception_handle, %eax
    call %eax       # A special call, preserves the 'eip' register
    pop %eax
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp      # Cleans up the pushed error code and pushed ISR number
	sti
    iret           # pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP!

.global irq0
.global irq1
.global irq2
.global irq3
.global irq4
.global irq5
.global irq6
.global irq7
.global irq8
.global irq9
.global irq10
.global irq11
.global irq12
.global irq13
.global irq14
.global irq15

irq0:
	cli
	push $0
	push $32
	jmp irq_common_stub

irq1:
	cli
	push $0
	push $33
	jmp irq_common_stub

irq2:
	cli
	push $0
	push $34
	jmp irq_common_stub

irq3:
	cli
	push $0
	push $35
	jmp irq_common_stub

irq4:
	cli
	push $0
	push $36
	jmp irq_common_stub

irq5:
	cli
	push $0
	push $37
	jmp irq_common_stub

irq6:
	cli
	push $0
	push $38
	jmp irq_common_stub

irq7:
	cli
	push $0
	push $39
	jmp irq_common_stub

irq8:
	cli
	push $0
	push $40
	jmp irq_common_stub

irq9:
	cli
	push $0
	push $41
	jmp irq_common_stub

irq10:
	cli
	push $0
	push $42
	jmp irq_common_stub

irq11:
	cli
	push $0
	push $43
	jmp irq_common_stub

irq12:
	cli
	push $0
	push $44
	jmp irq_common_stub

irq13:
	cli
	push $0
	push $45
	jmp irq_common_stub

irq14:
	cli
	push $0
	push $46
	jmp irq_common_stub

irq15:
	cli
	push $0
	push $47
	jmp irq_common_stub

.extern irq_handler
.extern kernel_loop

irq_common_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %esp, %eax
    push %eax
	mov $irq_handler, %eax
    call %eax
    pop %eax
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
	iret