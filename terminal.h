#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Check if the compiler thinks we are targeting the wrong operating system. */
#if defined(__linux__)
//#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif
 
/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
//#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif
  
/* Hardware text mode color constants. */
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};
  
size_t strlen(const char* str);

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;

void terminal_initialize(void);
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) ;
void terminal_putchar(char c);
int terminal_write(const char* data, size_t size);
int terminal_writestring(const char* data);
int terminal_writeuint32(const uint32_t data);
int terminal_writeuint64(const uint64_t data);
int terminal_writeptr(const void* data);
void terminal_setcolor(enum vga_color fg, enum vga_color bg);
int writeuint32(char* outBuf, const uint32_t data);
int writeuint64(char* outBuf, const uint64_t data);
int writestring(char* outBuf, const char* data);
int writeptr(char* buf, const void* data);
int sprintf(char* bufOut, char* format, ...);
char* sprintfToHeap(char* format, ...); 


