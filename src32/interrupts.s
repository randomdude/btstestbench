.code32

.intel_syntax noprefix

.global unknowninterrupt
unknowninterrupt:
call unknowninterrupt
iret

.global interrupt
interrupt:
	pushad
	pushf

	// push our argument, the interrupt number, so that interrupt_c can use it.
	push dword ptr [esp+(0x28)]		
	call interrupt_c

	popf
	popad
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
		add esp, 8
int0:	push 0
		call interrupt
		add esp, 8
		iret
int2:	push 2
		call interrupt
		add esp, 8
		iret
int3:	push 3
		call interrupt
		add esp, 8
		iret
int4:	push 4
		call interrupt
		add esp, 8
		iret
int5:	push 5
		call interrupt
		add esp, 8
		iret
int6:	push 6
		call interrupt
		add esp, 8
		iret
int7:	push 7
		call interrupt
		add esp, 8
		iret
int8:	push 8
		call interrupt
		add esp, 8
		iret
int9:	push 9
		call interrupt
		add esp, 8
		iret
int10:	push 10
		call interrupt
		add esp, 8
		iret
int11:	push 11
		call interrupt
		add esp, 8
		iret
int12:	push 12
		call interrupt
		add esp, 8
		iret
int13:	push 13
		call interrupt
		add esp, 8
		iret
int14:	push 14
		call interrupt
		add esp, 8
		iret
int15:	push 15
		call interrupt
		add esp, 8
		iret
int16:	push 16
		call interrupt
		add esp, 8
		iret
int17:	push 17
		call interrupt
		add esp, 8
		iret
int18:	push 18
		call interrupt
		add esp, 8
		iret
int19:	push 19
		call interrupt
		add esp, 8
		iret
int20:	push 20
		call interrupt
		add esp, 8
		iret

.att_syntax prefix
