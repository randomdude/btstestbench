#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tests.h"
#include "interrupts.h"
#include "heap.h"
#include "terminal.h"
#include "misc.h"

#define IA32_DEBUGCTL 0x01d9
#define IA32_MISC_ENABLE 0x01a0
#define IA32_DS_AREA 0x600
#define IA32_APIC_BASE 0x1b

// TODO: move this
interruptRecord* interrupts = NULL;
int interruptsSoFar = 0;

struct btm
{
	void* src;
	void* dst;
	void* flags;
} PACKED;

struct ds_area
{
	struct btm* btsBufferBase;
	struct btm* btsIndex;
	struct btm* btsAbsoluteMax;
	struct btm* btsInterruptThreshold;
	void* pebsBufferBase;
	void* pebsIndex;
	void* pebsAbsoluteMax;
	void* pebsInterruptThreshold;
	void* pebsCounterReset;
	void* reserved;
} PACKED;

testResult* test_capture(int loopCount);

testResult* test_ds(void)
{
	testResult* resultOut = allocHeap(sizeof(testResult));

	unsigned int leaf, eax, ebx, ecx, edx;
	leaf = 0x01;
	eax = ebx = ecx = edx = 0;
	__asm__ __volatile__("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (leaf), "c" (0));

	resultOut->didSucceed = ((edx >> 21) & 0x01);
	resultOut->extraInfo = sprintfToHeap("CPUID[1].edx = %x (expected bit 21 to be set; %s)\n", edx, resultOut->didSucceed ? "it is" : "it isn't");

	return resultOut;
}

testResult* test_misc_enable()
{
	testResult* resultOut = allocHeap(sizeof(testResult));

	uint32_t miscenables = rdmsr(IA32_MISC_ENABLE);

	bool perfMonAvailable = miscenables & (1 << 7);
	bool btsUnavailable = miscenables & (1 << 11);

	resultOut->didSucceed = false;
	if (perfMonAvailable && !btsUnavailable)
		resultOut->didSucceed = true;

	resultOut->extraInfo = sprintfToHeap("IA32_MISC_ENABLE.perfmon = %s\nIA32_MISC_ENABLE.BTS = %s\n",
		perfMonAvailable ? "available" : "unavailable",
		btsUnavailable ? "unavailable" : "available");

	return resultOut;
}


testResult* test_capture_interrupt()
{
	testResult* resultOut = allocHeap(sizeof(testResult));

	interruptsSoFar = 0;

	struct ds_area dsarea;
	memset(&dsarea, 0, sizeof(struct ds_area));
	struct btm btms[10];
	memset(btms, 0, sizeof(btms));

	// Set up the DS area
	dsarea.btsBufferBase = btms;
	dsarea.btsIndex = btms;
	dsarea.btsAbsoluteMax = &btms[10];
	dsarea.btsInterruptThreshold = &btms[4];	// Interrupt on the 5th trace.
	wrmsr_ptr(IA32_DS_AREA, &dsarea);

	// Set up the interrupts by programming the APIC.
	// First off, we need to enable the APIC by setting bit 11 in IA32_APIC_BASE.
	// We'll keep the APIC in xAPIC mode, not x2APIC (todo: test in x2APIC mode if available).
//	uint64_t apicBase = rdmsr(IA32_APIC_BASE);
//	apicBase |= (1 << 10);
	//apicBase |= (1 << 11);
//	wrmsr(IA32_APIC_BASE, apicBase);

	// "Set up the performance counter entry in the xAPIC LVT for fixed delivery and edge sensitive." (intel doc, 17.4.9.2, "Setting up the DS Save Area").
	// This means we need to configure the register found at offset 0x340 from the base of the apic (0xFEE00000). See "Intel® 64 Architecture x2APIC Specification".
	uint32_t* LVTPerfMonReg = (uint32_t*)0xfee00340;
	*LVTPerfMonReg = 0x400;	// NMI pls

	// tracing time - this time, with interrupts!
	uint64_t origdbgctl = rdmsr(IA32_DEBUGCTL);
	uint64_t dbgctl = origdbgctl | (1 << 6) | (1 << 7) | (1 << 8) | (0 << 9) | (0 << 10);
	wrmsr(IA32_DEBUGCTL, dbgctl);
	// the actual test payload, which runs during tracing
	extern void* testdest;
	extern void* testsrc;
	asm volatile(".intel_syntax noprefix;	\
	xor eax, eax; 				\
	inttestdest:				\
		inc eax;				\
		cmp eax, 7;				\
		jnz inttestdest;		\
								\
		.att_syntax prefix		\
	" : : : "eax");
	wrmsr(IA32_DEBUGCTL, origdbgctl);

	// We should've seen one interrupt, when we had just written the 5th trace. That interrupt
	// should be an NMI.
	int written = 0;
	resultOut->didSucceed = true;
	resultOut->extraInfo = allocHeap(1000);
	memset(resultOut->extraInfo, 0x00, 1000);
	resultOut->extraInfo[0] = 0;
	if (interruptsSoFar == 0)
	{
		written += sprintf(&resultOut->extraInfo[written], "No interrupt occured\n");
		resultOut->didSucceed = false;
	}
	if (interruptsSoFar > 1)
	{
		written += sprintf(&resultOut->extraInfo[written], "More than one interrupt occured\n", interrupts[0].interruptNum);
		resultOut->didSucceed = false;
	}

	if (interruptsSoFar == 1)
	{
		if (interrupts[0].interruptNum != 2)
		{
			written += sprintf(&resultOut->extraInfo[written], "Saw wrong interrupt: Expected interrupt 2 (NMI) but saw %x\n", interrupts[0].interruptNum);
			resultOut->didSucceed = false;
		}
		else
		{
			written += sprintf(&resultOut->extraInfo[written], "Saw correct interrupt: %x\n", interrupts[0].interruptNum);
		}
	}

	// We expect BTM 5 to be a branch to the NMI handler (int 2). If that's OK, we assume everything else is.
	if (btms[5].dst == int2)
	{
		written += sprintf(&resultOut->extraInfo[written], "BTM 5 dest %p, OK\n", btms[5].dst);
	}
	else
	{
		written += sprintf(&resultOut->extraInfo[written], "BTM 5 dest %p - incorrect, expected %p\n", btms[5].dst, int2);
		resultOut->didSucceed = false;
	}

	return resultOut;
}

testResult* test_capture_simple()
{
	testResult* resultOut = allocHeap(sizeof(testResult));

	// Here we do a simple capture.
	// We set up the DS area, with space for ten BTMs. We enable tracing, and run some assembly which
	// branches four times. We disable tracing and ensure we saw four BTMs with the addresses we expect.
	struct ds_area dsarea;
	memset(&dsarea, 0, sizeof(struct ds_area));
	struct btm btms[10];
	memset(btms, 0, sizeof(btms));

	// Set up the DS area
	dsarea.btsBufferBase = btms;
	dsarea.btsIndex = btms;
	dsarea.btsAbsoluteMax = &btms[10];
	dsarea.btsInterruptThreshold = 0;	// No interrupts for this test.
	wrmsr_ptr(IA32_DS_AREA, &dsarea);

	// tracing time!
	uint64_t origdbgctl = rdmsr(IA32_DEBUGCTL);
	uint64_t dbgctl = origdbgctl | (1 << 6) | (1 << 7) | (0 << 8) | (0 << 9) | (0 << 10);
	wrmsr(IA32_DEBUGCTL, dbgctl);
	// the actual test payload, which runs during tracing
	extern void* testdest;
	extern void* testsrc;
	asm volatile(".intel_syntax noprefix;	\
		xor eax, eax; 			\
	.global testdest;			\
	testdest:					\
		inc eax;				\
		cmp eax, 5;				\
	.global testsrc;			\
	testsrc:					\
		jnz testdest;			\
								\
		.att_syntax prefix		\
	" : : : "eax");
	wrmsr(IA32_DEBUGCTL, origdbgctl);

	// We should see 4 BTMs, from testsrc to testdst. We will check these and write human-readable status to extraInfo as we go.
	resultOut->extraInfo = allocHeap(1000);
	int written = 0;

	// Check how many BTMs we saw
	uint32_t btmCount = dsarea.btsIndex - dsarea.btsBufferBase;
	resultOut->didSucceed = true;
	if (btmCount != 4)
		resultOut->didSucceed = false;
	written += sprintf(&resultOut->extraInfo[written], "Number of traces: %x (%s)\n", btmCount, btmCount == 4 ? "as expected" : "incorrect");

	// And check each BTM.
	for (unsigned int n = 0; n < 10; n++)
	{
		if (n < 4)
		{
			// First loopCount BTMs should be set from testsrc to testdesc.
			if (btms[n].src != &testsrc || btms[n].dst != &testdest)
			{
				resultOut->didSucceed = false;
				written += sprintf(&resultOut->extraInfo[written], "Failed BTM %x: src %p dst %p\n", n, btms[n].src, btms[n].dst);
				written += sprintf(&resultOut->extraInfo[written], "            Expected src %p dst %p\n", &testsrc, &testdest);
			}
			else
			{
				written += sprintf(&resultOut->extraInfo[written], "good   BTM %x: src %p dst %p\n", n, btms[n].src, btms[n].dst);
			}
		}
		else
		{
			// All others should be untouched at zero.
			if (btms[n].src != 0 || btms[n].dst != 0)
			{
				resultOut->didSucceed = false;
				written += sprintf(&resultOut->extraInfo[written], "Failed BTM %x: src %p dst %p\n", n, btms[n].src, btms[n].dst);
				written += sprintf(&resultOut->extraInfo[written], "            Expected src %p dst %p\n", 0, 0);
			}
			else
			{
				written += sprintf(&resultOut->extraInfo[written], "good   BTM %x: src %p dst %p\n", n, btms[n].src, btms[n].dst);
			}
		}
	}
	if (interruptsSoFar != 0)
	{
		if (interruptsSoFar == 1)
			written += sprintf(&resultOut->extraInfo[written], "Saw interrupt: %x\n", interrupts[0].interruptNum);
		else
			written += sprintf(&resultOut->extraInfo[written], "Saw %d interrupts\n", interruptsSoFar);
		resultOut->didSucceed = false;
	}
	resultOut->extraInfo[written] = '\0';

	return resultOut;
}

testResult* test_capture_circular()
{
	testResult* resultOut = allocHeap(sizeof(testResult));

	// Here we do a simple capture.
	// We set up the DS area, with space for ten BTMs. We enable tracing, and run some assembly which
	// branches 20 times to 20 different addresses. We should see only the last ten addresses in our BTMs.
	struct ds_area dsarea;
	memset(&dsarea, 0, sizeof(struct ds_area));
	struct btm btms[10];
	memset(btms, 0, sizeof(btms));

	// Set up the DS area
	dsarea.btsBufferBase = btms;
	dsarea.btsIndex = btms;
	dsarea.btsAbsoluteMax = &btms[10];
	dsarea.btsInterruptThreshold = 0;	// No interrupts for this test.
	wrmsr_ptr(IA32_DS_AREA, &dsarea);

	// tracing time!
	uint64_t origdbgctl = rdmsr(IA32_DEBUGCTL);
	uint64_t dbgctl = origdbgctl | (1 << 6) | (1 << 7) | (0 << 8) | (0 << 9) | (0 << 10);
	wrmsr(IA32_DEBUGCTL, dbgctl);
	// the actual test payload, which runs during tracing
	extern void* jmp1;
	asm volatile(".intel_syntax noprefix;	\
		xor eax, eax; 			\
		jz jmp1;				\
	jmp1: jz jmp2;				\
	jmp2: jz jmp3;				\
	jmp3: jz jmp4;				\
	jmp4: jz jmp5;				\
	jmp5: jz jmp6;				\
	jmp6: jz jmp7;				\
	jmp7: jz jmp8;				\
	jmp8: jz jmp9;				\
	jmp9: jz jmp10;				\
	jmp10: jz jmp11;			\
	jmp11: jz jmp12;			\
	jmp12: jz jmp13;			\
	jmp13: jz jmp14;			\
	jmp14: jz jmp15;			\
	jmp15: jz jmp16;			\
	jmp16: jz jmp17;			\
	jmp17: jz jmp18;			\
	jmp18: jz jmp19;			\
	jmp19: jz jmp20;			\
	jmp20:						\
		.att_syntax prefix		\
	" : : : "eax");
	wrmsr(IA32_DEBUGCTL, origdbgctl);

	resultOut->extraInfo = allocHeap(1000);
	int written = 0;

	resultOut->didSucceed = true;
	uint32_t btmCount = dsarea.btsIndex - dsarea.btsBufferBase;
	if (btmCount != 10)
		resultOut->didSucceed = false;
	written += sprintf(&resultOut->extraInfo[written], "Number of traces: %x (%s)\n", btmCount, btmCount == 10 ? "as expected" : "incorrect");

	for (unsigned int n = 0; n < 10; n++)
	{
		uint16_t* jmps = (uint16_t*)&jmp1;
		void* expectedSrc = &jmps[ 9 + n];
		void* expectedDst = &jmps[10 + n];
		if (btms[n].src != expectedSrc || btms[n].dst != expectedDst)
		{
			written += sprintf(&resultOut->extraInfo[written], "Failed BTM %x: src %p dst %p\n", n, btms[n].src, btms[n].dst);
			written += sprintf(&resultOut->extraInfo[written], "            Expected src %p dst %p\n", expectedSrc, expectedDst);
		}
		else
		{
			written += sprintf(&resultOut->extraInfo[written], "good   BTM %x: src %p dst %p\n", n, btms[n].src, btms[n].dst);
		}
	}
	resultOut->extraInfo[written] = '\0';
	return resultOut;
}

