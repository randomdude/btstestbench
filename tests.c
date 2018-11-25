#include "tests.h"

#include "bts.h"

test availableTests[] = { 
		{ .name = "CPUID.DS", .testfunc = test_ds }, 
		{ .name = "IA32_MISC_ENABLE", .testfunc = test_misc_enable },
		{ .name = "Simple capture", .testfunc = test_capture_simple },
		{ .name = "Circular capture", .testfunc = test_capture_circular },
		{ .name = "Capture with interrupt", .testfunc = test_capture_interrupt },
		{ .name = 0 }
};