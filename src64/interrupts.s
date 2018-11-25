.code64

.intel_syntax noprefix

.global unknowninterrupt
unknowninterrupt:
call unknowninterrupt
iretq

.global interrupt
interrupt:
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	pushf

	mov rdi, qword ptr [rsp+(0x88)]		// our argument, the interrupt number.
	call interrupt_c

	popf
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret

.global int0
.global int1
.global int2
.global int3
.global int4
.global int5
.global int6
.global int7
.global int8
.global int9
.global int10
.global int11
.global int12
.global int13
.global int14
.global int15
.global int16
.global int17
.global int18
.global int19
.global int20
int1:	push 1
		call interrupt
		add rsp, 8
int0:	push 0
		call interrupt
		add rsp, 8
iretq
int2:	push 2
		call interrupt
		add rsp, 8
iretq
int3:	push 3
		call interrupt
		add rsp, 8
iretq
int4:	push 4
		call interrupt
		add rsp, 8
iretq
int5:	push 5
		call interrupt
		add rsp, 8
iretq
int6:	push 6
		call interrupt
		add rsp, 8
iretq
int7:	push 7
		call interrupt
		add rsp, 8
iretq
int8:	
		push 8
		call interrupt
		add rsp, 8
iretq
int9:	push 9
		call interrupt
		add rsp, 8
iretq
int10:	push 10
		call interrupt
		add rsp, 8
iretq
int11:	push 11
		call interrupt
		add rsp, 8
iretq
int12:	push 12
		call interrupt
		add rsp, 8
iretq
int13:	push 13
		call interrupt
		add rsp, 8
iretq
int14:	push 14
		call interrupt
		add rsp, 8
iretq
int15:	push 15
		call interrupt
		add rsp, 8
iretq
int16:	push 16
		call interrupt
		add rsp, 8
iretq
int17:	push 17
		call interrupt
		add rsp, 8
iretq
int18:	push 18
		call interrupt
		add rsp, 8
iretq
int19:	push 19
		call interrupt
		add rsp, 8
iretq
int20:	push 20
		call interrupt
		add rsp, 8
iretq

.att_syntax prefix
