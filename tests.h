#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct testResult
{
	bool didSucceed;
	char* extraInfo;
}; typedef struct testResult testResult;

typedef testResult*(*testFunction)(void);

struct test
{
	testFunction testfunc;
	char* name;
	testResult* result;
}; typedef struct test test;

extern test availableTests[];
