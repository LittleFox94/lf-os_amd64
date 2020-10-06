#ifndef _LFOSTEST_H_INCLUDED
#define _LFOSTEST_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

typedef uint64_t ptr_t;

#define TEST_FUNCTIONS \
    testFunctionT(size_t, eq) \
    testFunctionT(size_t, ne) \
    testFunctionT(ptr_t, eq) \
    testFunctionT(ptr_t, ne)

typedef struct {
#define testFunctionT(type, name) void(*name ## _ ## type)(type expected, type actual, const char* message);
    TEST_FUNCTIONS
#undef testFunctionT
} TestUtils;

typedef void(*TestMain)(TestUtils* utils);

#endif
