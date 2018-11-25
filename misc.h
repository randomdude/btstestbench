#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define NAKED  __attribute__((naked))
#define PACKED __attribute__((packed))

size_t strlen(const char* str);
void wrmsr_ptr(uint32_t msr_id, void* msr_value);
uint64_t rdmsr(uint32_t msr_id);
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t val);
void io_wait(void);
void reboot();
void memset(void* stuff, uint8_t newval, unsigned int len);
bool memcmp(void* stuff1, void* stuff2, unsigned int len);
void initAndDisablePIC();

__attribute__((always_inline)) inline void wrmsr(uint32_t msr_id, uint64_t msr_value)
{
#ifdef __x86_64
	uint32_t low = msr_value & 0xFFFFFFFF;
	uint32_t high = msr_value >> 32;
	asm volatile ("wrmsr" : : "c"(msr_id), "a"(low), "d"(high));
#else
	__asm__ __volatile__("wrmsr" : : "c" (msr_id), "A" (msr_value));
#endif
}
