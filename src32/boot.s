/* Declare constants for the multiboot header. */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/* 
Declare a multiboot header that marks the program as a kernel. These are magic
values that are documented in the multiboot standard. The bootloader will
search for this signature in the first 8 KiB of the kernel file, aligned at a
32-bit boundary. The signature is in its own section so the header can be
forced to be within the first 8 KiB of the kernel file.
*/
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/*
The multiboot standard does not define the value of the stack pointer register
(esp) and it is up to the kernel to provide a stack. This allocates room for a
small stack by creating a symbol at the bottom of it, then allocating 16384
bytes for it, and finally creating a symbol at the top. The stack grows
downwards on x86. The stack is in its own section so it can be marked nobits,
which means the kernel file is smaller because it does not contain an
uninitialized stack. The stack on x86 must be 16-byte aligned according to the
System V ABI standard and de-facto extensions. The compiler will assume the
stack is properly aligned and failure to align the stack will result in
undefined behavior.
*/
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .text
.global _start
.type _start, @function
.intel_syntax noprefix
_start:
	mov esp, 0x00220000

	// We will now set up memory mapping and enable paging. We'll put the base of the table at 0x1000.
	mov edi, 0x1000
	mov cr3, edi

	// We'll put the page directory at 0x1000. It has 1024/0x400 entries since it is ten bits. 
	// We will use 4mb pages, identity mapping the whole 32bit VA range.
	mov ecx, 0x400		// 0x400 entires
	mov edx, 0x1000		// page directory starts at 0x1000
	mov ebx, 0x0		// first page of physical memory is zero.
.fillPageDirectory:
	mov DWORD PTR [edx], ebx
	or  DWORD PTR [edx], 0x83	// Size (4mb per page) | present | writable
	add ebx, 0x400000			// Next page is 4MB away.
	add edx, 4
	loop .fillPageDirectory 

	// Set cr4.PSE, to indicate we want to use 4MB pages
	mov eax, cr4
	or eax, 1 << 4
	mov cr4, eax

	// Enable paging by setting cr0.PE
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	call kernel_main

	cli
1:	hlt
	jmp 1b

.att_syntax prefix

/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start

