#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "terminal.h"
#include "heap.h"

uint8_t terminal_color;
uint16_t* terminal_buffer;

int terminal_writevalue(const uint64_t data, int hexDigitsToSkip);
int writevalue(char* bufOut, const uint64_t data, int sizeInHexDigits);
int sprintfToVAList(va_list va, char* bufferOut, char* format);

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) 
{
	return fg | bg << 4;
}
 
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) 
{
	return (uint16_t) uc | (uint16_t) color << 8;
}

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}
 
void terminal_setcolor(enum vga_color fg, enum vga_color bg)
{
	terminal_color = vga_entry_color(fg, bg);
}
 
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}
 
void terminal_putchar(char c) 
{
  if (c != '\n')
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH || c == '\n') {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
	}
}
 
int terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
	return size;
}

int terminal_writestring(const char* data) 
{
	return terminal_write(data, strlen(data));
}

int writestring(char* outBuf, const char* data)
{
	unsigned int len = strlen(data) + 1;
	if (outBuf != NULL)
	{
		for (unsigned int n = 0; n < len; n++)
			outBuf[n] = data[n];
	}
	return len;
}

int terminal_writeuint32(const uint32_t data)
{
	terminal_write("0x", 2);
	char outBuf[8];
	int bytesWritten = writeuint32(outBuf, data);
	return terminal_write(outBuf, bytesWritten) + 2;
}

int writeuint32(char* outBuf, const uint32_t data)
{
	return writevalue(outBuf, (uint64_t)data, 8);
}

int terminal_writeuint64(const uint64_t data)
{
	terminal_write("0x", 2);
	char outBuf[16];
	int bytesWritten = writeuint64(outBuf, data);
	return terminal_write(outBuf, bytesWritten) + 2;
}

int writeuint64(char* bufOut, const uint64_t data)
{
	return writevalue(bufOut, data, 0);
}

int terminal_writeptr(const void* data)
{
#ifdef __x86_64
	return terminal_writeuint64((uint64_t)data);
#else
	return terminal_writeuint32((uint32_t)data);
#endif
}

int writeptr(char* buf, const void* data)
{
#ifdef __x86_64
	return writeuint64(buf, (uint64_t)data);
#else
	return writeuint32(buf, (uint32_t)data);
#endif
}

int terminal_writevalue(const uint64_t data, int sizeInHexDigits)
{
	terminal_write("0x", 2);
	char outBuf[16];
	int bytesWritten = writevalue(outBuf, data, sizeInHexDigits);
	return terminal_write(outBuf, bytesWritten) + 2;
}

int writevalue(char* bufOut, const uint64_t data, int sizeInHexDigits)
{
	int toRet = 0;

	uint8_t nibbleToPrint;
	char* digits = "0123456789abcdef";

	if (sizeInHexDigits == 0)
		sizeInHexDigits = (sizeof(uint64_t) * 2);

	for (signed int n = sizeInHexDigits - 1; n >= 0; n--)
	{
		nibbleToPrint = (data >> (n*4)) & 0x0f;
		if (bufOut != NULL)
			bufOut[toRet] = (digits[nibbleToPrint]);
		toRet++;
	}
	return toRet;
}

char* sprintfToHeap(char* format, ...)
{
	va_list va, va2;
	va_start(va, format);
	va_copy(va2, va);
	int strLen = sprintfToVAList(va, NULL, format);
	va_end(va);

	char* toRet = allocHeap(strLen + 1);
	sprintfToVAList(va2, toRet, format);
	toRet[strLen] = 0;
	va_end(va2);
	return toRet;
}

int sprintf(char* bufferOut, char* format, ...)
{
	va_list va;
	va_start(va, format);
	int toRet = sprintfToVAList(va, bufferOut, format);
	va_end(va);
	return toRet;
}

int sprintfToVAList(va_list va, char* bufferOut, char* format)
{
	int bytesWritten = 0;

	char* pos = format;
	bool nextIsEscaped = false;
	int nextLen = 32;
	while (*pos != 0)
	{
		if (nextIsEscaped)
		{
			if (*pos == 'l')
			{
				nextLen += 32;
			}
			else
			{
				int written = 0;
				switch (*pos)
				{
				case '%':
					terminal_putchar('%');
					bytesWritten++;
					break;
				case 's':
					written = writestring(bufferOut, va_arg(va, char*));
					// Knock off that terminating null
					written--;
					break;
				case 'p':
					written = writeptr(bufferOut, va_arg(va, void*));
					break;
				case 'x':
					if (nextLen == 32)
					{
						written = writeuint32(bufferOut, va_arg(va, uint32_t));
					}
					else
					{
						written = writeuint64(bufferOut, va_arg(va, uint64_t));
					}
					break;
				}
				bytesWritten += written;
				if (bufferOut != NULL)
					bufferOut += written;
				nextIsEscaped = false;
			}
		}
		else
		{
			if (*pos == '%')
			{
				nextIsEscaped = true;
				nextLen = 32;
			}
			else
			{
				if (bufferOut != NULL)
				{
					bufferOut[0] = pos[0];
					bufferOut++;
				}
				bytesWritten++;
			}
		}
		pos++;
	}
	return bytesWritten;
}
