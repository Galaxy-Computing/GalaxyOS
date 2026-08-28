.section .text

# .extern initialize_standard_library
.global _start
_start:
	# This puts all our arguments on the stack, including argc and argv at the top
	mov $14, %eax
	int $0x80

	mov %esp, %ebp

	# Tell the kernel where __procerrhandler is
	mov __procerrhandler, %ecx
	mov $11, %eax
	int $0x80

	# Prepare signals, memory allocation, stdio and such.
	# call initialize_standard_library

	# Run the global constructors.
	call _init

	# Run main
	call main

	# Terminate the process with the exit code.
	push %eax
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

	
	
