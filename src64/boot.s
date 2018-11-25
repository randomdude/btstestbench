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

// Memory map:
// 0x00001000 - 0x0000		: PML4 through to page tables
// 0x00100000 -				: this code
// 0x00200000 - 0x00210000  : heap (see heap.c)
//              0x00220000  : stack
// 0x01400000 -				: unused?
// 0xffe00000 - 0xffe01000	: APIC MMIO

.section .text
.code32
.global _start
_start:
.intel_syntax noprefix
	mov esp, 0x00220000

	// Gotta go to long mode.
	// Set up a PML4 table at 0x1000, and select it via cr3.
	mov edi, 0x1000
	// Clear the ranges we'll be using
	xor eax, eax
	mov ecx, 0x4000
	rep stosd

	// PML4 at 0x1000
	//   +0x00 (ranges 0x00000000-0x40000000) -> PDPTE at 0x2000
	// PDPTE at 0x2000 (controlling 0x00000000-0x40000000):
	//   +0x00 (ranges 0x00000000-0x40000000) -> PD at 0x3000
	//   +0x03 (ranges 0xC0000000-0xFFFFFFFF) -> PD at 0xF000
	//
	// PDE at 0x3000:
	//	 +0x00 (ranges 0x00000000-0x00200000) -> PT at 0x4000
	//	 +0x01 (ranges 0x00200000-0x00400000) -> PT at 0x5000
	//   ...
	//	 +0x09 (ranges 0x01400000-0x01600000) -> PT at 0xD000
	// PT at 0x4000:
	//   -> physical mem at 0x00000000-0x00001000
	// PT at 0x5000:
	//   -> physical mem at 0x00001000-0x00002000
	// ...
	// PT at 0xD000:
	//   -> physical mem at 0x0000D000-0x0000E000
	//
	// PDE at 0xF000:
	//	 +0x1bf (ranges 0xFEE00000-0xFEFFFFFF) -> PT at 0x10000
	// PT at 0x10000:
	//   -> physical mem at 0xFFE01000-0xFFE10000

	// Set the PML4 entry. Set 'present' and 'writable', so that's bits 0 and 1
	mov DWORD PTR [0x1000], 0x2003

	// The first entry in PDPTE at 0x2000 points to PD at 0x3000
	mov DWORD PTR [0x2000], 0x3003

	// Now our PDEs, and a page table for each.
	mov edx, 0x4000		// the first page table starts at 0x4000
	mov edi, 0x3000		// PDE starts at 0x3000
	mov ecx, 10			// 10 PDEs pls
	mov ebx, 0x00		// mapped memory starts at 0x00

.SetPDE:
	mov DWORD PTR [edi], edx
	add DWORD PTR [edi], 0x03	// Set bits 0 and 1 like before
	push ecx
	push edi

	// fill the page table. There are 512 entries of eight
	// bytes each, and start at edx. The physical memory they point to is in
	// ebx.
	mov ecx, 512		// set 512 entries
.SetEntry:
	mov DWORD PTR [edx+0], ebx
	add DWORD PTR [edx+0], 0x03	// present | writable
	mov DWORD PTR [edx+4], 0
	add ebx, 0x1000
	add edx, 8
	loop .SetEntry

	pop edi
	pop ecx
	add edi, 8	// move to next PDE
	loop .SetPDE

	// So that's our rwx ranges set up. Next, set an entry for the APIC MMIO range, which starts at 0xfee00000.
	// Clear the range we need.
	mov edi, 0xF000
	xor eax, eax
	mov ecx, 0x4000
	rep stosd

	mov DWORD PTR [0x2018], 0xF003
	mov DWORD PTR [0xffb8], 0x10003
	// Fill that page table up
	mov edx, 0x10000
	mov ebx, 0xfee00000
	mov ecx, 512
.SetPTEntry:
	mov DWORD PTR [edx+0], ebx
	add DWORD PTR [edx+0], 0x03	// present | writable
	mov DWORD PTR [edx+4], 0
	add ebx, 0x1000
	add edx, 8
	loop .SetPTEntry

	mov edi, 0x1000
	mov cr3, edi

	// Now that paging is set up, we can enable it.

	// Set cr4.PAE
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	// Set IA32_EFER.LME
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	and eax, ~(1<<4)
	wrmsr

	// Enable paging by setting cr0.PE
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax
	
	// At this point, we're in ia-32e "compatibility mode". Everything is 64bit,
	// except our code - since we're in a 32bit code page (as defined by the gdt).
	// Finally, we can jump to a 64bit code page, and be "properly" 64bit.
	lgdt [gdtrPointer]
	jmp 0x08:Realm64

	.align 8
GDT64:
    .Null:
    .word 0
    .word 0
    .byte 0
    .byte 0
    .byte 0
    .byte 0
    .Code:
    .word 0x0000		// base 0:15
    .word 0				// limit 0:15
    .byte 0x00			// base 24:31
    .byte 0b10011010	// access ( PR | EX | RW )
    .byte 0b10101111	// flags, limit 16:19
    .byte 0x00			// base 16:23
    .Data:
    .word 0x0000		// base 0:15
    .word 0				// limit 0:15
    .byte 0x00			// base 24:31
    .byte 0b10010010	// access ( PR | RW )
    .byte 0b10101111	// flags, limit 16:19
    .byte 0x00			// base 16:23
    gdtrPointer:
    .word $ - GDT64 - 1
    .long GDT64


.code64

Realm64:
	// Finally, we are in 64bit mode!
	mov esp, 0x00220000
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

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

