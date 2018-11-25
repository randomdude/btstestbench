#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "misc.h"

// PIC defines, taken from osdev
#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)
#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */
#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */


size_t strlen(const char* str)
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

void wrmsr_ptr(uint32_t msr_id, void* msr_value)
{
#ifdef __x86_64
	wrmsr(msr_id, (int64_t)msr_value);
#else
	wrmsr(msr_id, (int32_t)msr_value);
#endif
}

uint64_t rdmsr(uint32_t msr_id)
{
	uint64_t msr_value;
	__asm__ __volatile__("rdmsr" : "=A" (msr_value) : "c" (msr_id));
	return msr_value;
}

uint8_t inb(uint16_t port)
{
	uint8_t ret;
	__asm__ __volatile__("inb %1, %0"
		: "=a"(ret)
		: "Nd"(port));
	return ret;
}

void outb(uint16_t port, uint8_t val)
{
	__asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

void io_wait(void)
{
	/* Port 0x80 is used for 'checkpoints' during POST. */
	/* The Linux kernel seems to think it is free for use :-/ */
	asm("outb %%al, $0x80" : : "a"(0));
	/* %%al instead of %0 makes no difference.  TODO: does the register need to be zeroed? */
}

void reboot()
{
	uint8_t good = 0x02;
	while (good & 0x02)
		good = inb(0x64);
	outb(0x64, 0xFE);
	while (true);
}

void memset(void* stuff, uint8_t newval, unsigned int len)
{
	for (unsigned int n = 0; n<len; n++)
		((unsigned char*)stuff)[n] = newval;
}

bool memcmp(void* stuff1, void* stuff2, unsigned int len)
{
	for (unsigned int n = 0; n<len; n++)
		if (((unsigned char*)stuff1)[n] != ((unsigned char*)stuff2)[n])
			return false;
	return true;
}

void initAndDisablePIC()
{
	// We disable the PIC by masking everything. We still have to remap it past the BIOS interrupt range, though, so we put it starting at 0x80.
	// To do this, we do the whole PIC initialisation sequence. 
	// Thanks to osdev once again - this function is almost exactly taken from https://wiki.osdev.org/PIC.
	outb(PIC1_COMMAND, ICW1_INIT + ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT + ICW1_ICW4);
	io_wait();
	outb(PIC1_DATA, 0x80);                 // ICW2: Master PIC vector offset
	io_wait();
	outb(PIC2_DATA, 0x80);                 // ICW2: Slave PIC vector offset
	io_wait();
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();

	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	// Now we can mask everything off.
	outb(0xa1, 0xff);
	outb(0x21, 0xff);
}