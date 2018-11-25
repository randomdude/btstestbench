#include <stdbool.h>

#include "terminal.h"
#include "heap.h"

struct heapblock
{
	struct heapblock* next;
	bool isFree;
	int allocationSize;
	unsigned char userdata[0];
}; typedef struct heapblock heapblock;

heapblock* heapBase;

void initHeap()
{
	heapBase = (heapblock*)0x00200000;
	heapBase->next = 0;
	heapBase->isFree = true;
	heapBase->allocationSize = 0x10000 - sizeof(heapblock);
}

void* allocHeap(int requestedSize)
{
	heapblock* candidate = heapBase;
	do
	{
		if (!candidate->isFree || candidate->allocationSize < requestedSize)
		{
			candidate = candidate->next;
			continue;
		}
		// OK, this block is free and can accomodate the request. We need to size if appropriately, mark it in use, and - 
		// if the requested block is smaller than the block we're going to use - initialize the block after it to be a free block.
		if (candidate->allocationSize == requestedSize)
		{
			candidate->isFree = false;
			return candidate->userdata;
		}
		heapblock* newBlock = (heapblock*)&candidate->userdata[requestedSize];
		newBlock->isFree = true;
		newBlock->allocationSize = candidate->allocationSize - (requestedSize + sizeof(heapblock));
		newBlock->next = candidate->next;

		candidate->next = newBlock;
		candidate->allocationSize = requestedSize;
		candidate->isFree = false;

		return candidate->userdata;
	} while (candidate != 0);

	return NULL;
}

void freeHeap(void* userdata)
{
	heapblock* toFree = (heapblock*)(((char*)userdata) - sizeof(heapblock));
	toFree->isFree = true;
}

void dumpHeap(void)
{
	heapblock* candidate = heapBase;
	do
	{
		terminal_writestring("Block at ");
		terminal_writeptr(candidate);
		if (candidate->isFree)
			terminal_writestring(": free");
		else
			terminal_writestring(": busy");
		terminal_writestring(" size ");
		terminal_writeuint32(candidate->allocationSize);
		terminal_writestring("\n");
		candidate = candidate->next;
	} while (candidate != 0);
}