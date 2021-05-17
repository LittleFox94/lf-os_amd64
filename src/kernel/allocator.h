#ifndef _ALLOCATOR_H_INCLUDED
#define _ALLOCATOR_H_INCLUDED

#include <stdint.h>

struct allocator;

typedef struct allocator {
    void*(*alloc)(struct allocator* alloc, size_t size);
    void(*dealloc)(struct allocator* alloc, void* mem);

    uint64_t tag;
} allocator_t;

extern allocator_t kernel_alloc;

#endif
