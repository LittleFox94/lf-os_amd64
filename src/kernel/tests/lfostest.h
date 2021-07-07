#ifndef _LFOSTEST_H_INCLUDED
#define _LFOSTEST_H_INCLUDED

#include <gtest/gtest.h>

typedef uint64_t ptr_t;

#include "../allocator.h"

#if __cplusplus
extern "C" allocator_t* alloc;
#else
extern allocator_t* alloc;
#endif

#endif
