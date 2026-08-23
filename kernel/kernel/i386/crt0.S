.section .text

# .extern initialize_standard_library
.global _start
_start:
	# Set up end of the stack frame linked list.
	mov $0, %ebp
	push %ebp # eip=0
	push %ebp # ebp=0
	mov %esp, %ebp

	# Tell the kernel where __procerrhandler is
	mov __procerrhandler, %ecx
	mov $11, %eax
	int $0x80

	# We need those in a moment when we call main.
	push %ecx
	push %edx

	# Prepare signals, memory allocation, stdio and such.
	# call initialize_standard_library

	# Run the global constructors.
	call _init

	# Restore argc and argv.
	pop %edi
	pop %esi

	# Run main
	call main

	# Terminate the process with the exit code.
	mov %eax, %ecx
	call exit
.size _start, . - _start

.extern libc_procerrhandler
__procerrhandler:
	# We need to make sure the processor state is saved before calling into C
	pusha

	# The kernel would have placed the error code before the return address on the stack
	push -36(%esp)
	call libc_procerrhandler

	# Tell the kernel that we've finished handling the error
	mov %eax, %ecx
	mov $10, %eax
	int $0x80

	# Afterwards, we need to clean everything up
	add $4, %esp
	popa
	add $4, %esp
	ret

	
	
