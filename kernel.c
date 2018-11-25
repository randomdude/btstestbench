#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "terminal.h"
#include "interrupts.h"
#include "misc.h"
#include "tests.h"
#include "heap.h"

#ifdef __x86_64
struct IDTDescr {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_attr;
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t zero_again;
} PACKED;

struct idtr
{
	uint16_t len;
	struct IDTDescr* base;
} PACKED;

struct idtr theidtreg;
struct IDTDescr idt[255];

void makeidtline(struct IDTDescr* lineout, uint16_t selector, void* offset)
{
	uint64_t offsetAddress = (uint64_t)offset;

	lineout->offset_low = offsetAddress & 0x0000ffff;
	lineout->selector = selector;
	lineout->zero = 0;
	//  lineout->type_attr=0x8f;	// present, 32bit trap
	lineout->type_attr = 0x8e;	// present, 32bit interrupt
	lineout->offset_mid = (offsetAddress >> 16) & 0x0000ffff;
	lineout->offset_high = (offsetAddress >> 32) & 0xffffffff;
	lineout->zero_again = 0;
}

#else	//__x86_64

struct IDTDescr {
	uint16_t offset_1;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_attr;
	uint16_t offset_2;
} PACKED;

struct idtr
{
	uint16_t len;
	struct IDTDescr* base;
} PACKED;

void makeidtline(struct IDTDescr* lineout, uint16_t selector, void* offset)
{
	uint32_t offsetAddress = (uint32_t)offset;
	
	lineout->offset_2 = (offsetAddress >> 16) & 0x0000ffff;
  lineout->selector=selector;
  lineout->zero=0;
//  lineout->type_attr=0x8f;	// present, 32bit trap
  lineout->type_attr=0x8e;	// present, 32bit interrupt
  lineout->offset_1 = offsetAddress & 0x0000ffff;
}

#endif

struct idtr theidtreg;
struct IDTDescr idt[255];

void unknowninterrupt_c(void)
{
 	terminal_writestring("Unexpected interrupt");
}

void __attribute__((stdcall)) interrupt_c(int interruptnum)
{
//	terminal_writestring("Oh no an interrupt: ");
//	terminal_writeuint32(interruptnum);
//	while (true) {}
	if (interruptsSoFar != MAX_INTERRUPT_RECORDS)
	{
		interrupts[interruptsSoFar].interruptNum = interruptnum;
		interruptsSoFar++;
	}
}

void kernel_main()
{
	terminal_initialize();
 	terminal_writestring("BTS testbench\n\n");
	initHeap();

	// OK, so now we need some interrupts. Gotta set up that IDT.
	theidtreg.len = (255 * sizeof(struct IDTDescr))-1;
	theidtreg.base = idt;

	// Our interrupt handlers will store interrupt statuses here.
	interrupts = allocHeap(sizeof(interruptRecord) * MAX_INTERRUPT_RECORDS);
	interruptsSoFar = 0;

	// Point everything to the 'unknown interrupt' handler, located with CS 0x08.
	for (int n = 0; n<255; n++)
		makeidtline(&idt[n], 0x08, unknowninterrupt);
	makeidtline(&idt[0], 0x08, int0);
	makeidtline(&idt[1], 0x08, int1);
	makeidtline(&idt[2], 0x08, int2);
	makeidtline(&idt[3], 0x08, int3);
	makeidtline(&idt[4], 0x08, int4);
	makeidtline(&idt[5], 0x08, int5);
	makeidtline(&idt[6], 0x08, int6);
	makeidtline(&idt[7], 0x08, int7);
	makeidtline(&idt[8], 0x08, int8);
	makeidtline(&idt[9], 0x08, int9);
	makeidtline(&idt[10], 0x08, int10);
	makeidtline(&idt[11], 0x08, int11);
	makeidtline(&idt[12], 0x08, int12);
	makeidtline(&idt[13], 0x08, int13);
	makeidtline(&idt[14], 0x08, int14);
	makeidtline(&idt[15], 0x08, int15);
	makeidtline(&idt[16], 0x08, int16);
	makeidtline(&idt[17], 0x08, int17);
	makeidtline(&idt[18], 0x08, int18);
	makeidtline(&idt[19], 0x08, int19);
	makeidtline(&idt[19], 0x08, int20);
	//	makeidtline(&thegdtreg.base[0x02], 0x10, &NMIinterrupt);
	asm("lidt %0" : : "m"(theidtreg));

	// Disable the legacy PIC, since we'll be using the APIC. 
	initAndDisablePIC();

	// And enable interrupts.
	asm("sti");


	// Run all the tests
	int testIdx = 0;
	bool allTestsPassed = true;
	while (availableTests[testIdx].name != NULL)
	{
		test testds = availableTests[testIdx];

		terminal_writestring("Now running test '");
		terminal_writestring(testds.name);
		terminal_writestring("'..\n");

		availableTests[testIdx].result = testds.testfunc();
		if (!availableTests[testIdx].result->didSucceed)
			allTestsPassed = false;
		testIdx++;
	}
	int testCount = testIdx - 1;
	
	// Now give the user the results and a nice menu.
	int selectedTestIndex = 0;
	int testDetailPos = 0;
	while (true)
	{
		int testIdx = 0;
		terminal_initialize();
		while (availableTests[testIdx].name != NULL)
		{
			test testds = availableTests[testIdx];

			terminal_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
			terminal_writestring("          ");

			enum vga_color bgCol = VGA_COLOR_BLACK;
			if (selectedTestIndex == testIdx)
			{
				bgCol = VGA_COLOR_BLUE;
				terminal_setcolor(VGA_COLOR_WHITE, bgCol);
				terminal_putchar(0xaf);
			}
			else
			{
				terminal_putchar(' ');
			}

			terminal_setcolor(VGA_COLOR_LIGHT_GREY, bgCol);
			terminal_writestring("     Test '");
			terminal_writestring(testds.name);
			terminal_writestring("' : ");
			while (terminal_column < 60)
				terminal_putchar(' ');
			if (testds.result->didSucceed)
			{
				terminal_setcolor(VGA_COLOR_WHITE, VGA_COLOR_GREEN);
				terminal_writestring(" OK ");
			}
			else
			{
				terminal_setcolor(VGA_COLOR_WHITE, VGA_COLOR_RED);
				terminal_writestring("FAIL");
			}

			if (selectedTestIndex == testIdx)
			{
				terminal_setcolor(VGA_COLOR_WHITE, bgCol);
				while (terminal_column < VGA_WIDTH - 10)
					terminal_putchar(' ');
				terminal_putchar(0xae);
			}

			terminal_writestring("\n");

			terminal_setcolor(VGA_COLOR_LIGHT_GREY, bgCol);
			testIdx++;
		}
		// Print the output of the currently-selected test
		terminal_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
		terminal_putchar('\n');
		for (unsigned int n = 0; n < 5; n++)
			terminal_putchar(0xCD);
		terminal_putchar(0xB9);
		terminal_putchar(' ');
		terminal_writestring(availableTests[selectedTestIndex].name);
		terminal_putchar(' ');
		terminal_putchar(0xCC);
		while (terminal_column < VGA_WIDTH - 1)
			terminal_putchar(0xCD);
		terminal_putchar('\n');

		// Skip a few lines if requested
		char* trimmedInfo = availableTests[selectedTestIndex].result->extraInfo;
		int linesSkipped = 0;
		while (*trimmedInfo != 0 && linesSkipped != testDetailPos)
		{
			if (trimmedInfo[0] == '\n')
				linesSkipped++;
			trimmedInfo++;
		}
		if (*trimmedInfo == 0)
			testDetailPos = linesSkipped - 1;
		// Print ten lines, clipping and padding where needed
		int linesLeft = 10;
		while (*trimmedInfo != 0 && linesLeft > 0)
		{
			terminal_putchar(*trimmedInfo);
			if (*trimmedInfo == '\n')
				linesLeft--;
			trimmedInfo++;
		}
		bool isMore = (linesLeft == 0);
		while (linesLeft > 0)
		{
			terminal_putchar('\n');
			linesLeft--;
		}
		// Now a nice line to close off the test detail.
		terminal_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
		for (unsigned int n = 0; n < 5; n++)
			terminal_putchar(0xCD);
		if (isMore)
		{
			terminal_putchar(0xB9);
			terminal_writestring(" PAGE UP / PAGE DOWN FOR MORE ");
			terminal_putchar(0xCC);
		}
		while (terminal_column < VGA_WIDTH - 1)
			terminal_putchar(0xCD);
		terminal_putchar('\n');

		// If there's space, put a status bar and help bar at the bottom.
		if (terminal_row != VGA_HEIGHT - 2)
		{
			// status bar at second-from-bottom row
			while (terminal_row != VGA_HEIGHT - 2)
				terminal_putchar('\n');			
			if (allTestsPassed)
			{
				terminal_setcolor(VGA_COLOR_WHITE, VGA_COLOR_GREEN);
				terminal_writestring("Overall status: ALL TESTS PASSED; HAPPY DAYS   :)");
			}
			else
			{
				terminal_setcolor(VGA_COLOR_WHITE, VGA_COLOR_RED);
				terminal_writestring("Overall status: AT LEAST ONE TEST DID NOT PASS :(");
			}
			while (terminal_column != VGA_WIDTH - 1)
				terminal_putchar(' ');

			// And help bar at the bottom row.
			terminal_setcolor(VGA_COLOR_BLUE, VGA_COLOR_LIGHT_GREY);
			terminal_writestring("Help: [ESC] to reboot; arrow keys to move up/down; PGUP/PGDOWN to scroll detail");
			while (terminal_column != VGA_WIDTH - 1)
				terminal_putchar(' ');
			terminal_putchar(' ');
		}

		// Wait for the user to hit a key
		while (!(inb(0x64) & 1));
		unsigned char scanCodeIn = inb(0x60);
		if (scanCodeIn == 0xe0)
			scanCodeIn = inb(0x60);
		switch (scanCodeIn)
		{
		case 0x01: 
			// Escape.
			reboot();
			break;
		case 0x48:	// arrow up
			selectedTestIndex--;
			testDetailPos = 0;
			break;
		case 0x50:	// arrow down
			selectedTestIndex++;
			testDetailPos = 0;
			break;
		case 0x49:	// page up
			if (testDetailPos > 0)
				testDetailPos--;
			break;
		case 0x51:	// page down
			testDetailPos++;
			break;
		}
		if (selectedTestIndex < 0)
			selectedTestIndex = testCount ;
		if (selectedTestIndex > testCount )
			selectedTestIndex = 0;
	}
}
