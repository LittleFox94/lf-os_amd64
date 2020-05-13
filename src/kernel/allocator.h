#ifndef _ALLOCATOR_H_INCLUDED
#define _ALLOCATOR_H_INCLUDED

#include <stdint.h>

typedef void*(allocator_t)(size_t size);
typedef void(deallocator_t)(void* ptr);

#endif
