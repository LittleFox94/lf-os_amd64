#ifndef _LFOSTEST_H_INCLUDED
#define _LFOSTEST_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#ifndef _TESTRUNNER
void* malloc(size_t);
void free(void*);
#else
#include <stddef.h>
typedef uint64_t ptr_t;
#endif

#define TEST_FUNCTIONS \
    testFunctionT(size_t, eq) \
    testFunctionT(size_t, ne) \
    testFunctionT(size_t, lt) \
    testFunctionT(size_t, gt) \
    testFunctionT(int, eq) \
    testFunctionT(int, ne) \
    testFunctionT(int, lt) \
    testFunctionT(int, gt) \
    testFunctionT(uint8_t, eq) \
    testFunctionT(uint8_t, ne) \
    testFunctionT(uint8_t, lt) \
    testFunctionT(uint8_t, gt) \
    testFunctionT(uint64_t, eq) \
    testFunctionT(uint64_t, ne) \
    testFunctionT(uint64_t, lt) \
    testFunctionT(uint64_t, gt) \
    testFunctionT(ptr_t, eq) \
    testFunctionT(ptr_t, ne) \
    testFunctionT(bool, eq) \
    testFunctionT(bool, ne)

typedef struct {
#define testFunctionT(type, name) void(*name ## _ ## type)(type expected, type actual, const char* message);
    TEST_FUNCTIONS
#undef testFunctionT
} TestUtils;

typedef void(*TestMain)(TestUtils* utils);

#include "../allocator.h"

#if __cplusplus
extern "C" allocator_t* alloc;
#else
extern allocator_t* alloc;
#endif

#endif
