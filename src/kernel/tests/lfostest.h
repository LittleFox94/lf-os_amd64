#ifndef _LFOSTEST_H_INCLUDED
#define _LFOSTEST_H_INCLUDED

#define LF_OS_TESTING

#include <gtest/gtest.h>

typedef uint64_t ptr_t;

#include "../allocator.h"

extern allocator_t* alloc;

#endif
