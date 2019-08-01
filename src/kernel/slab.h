#ifndef _SLAB_H_INCLUDED
#define _SLAB_H_INCLUDED

#include <stdint.h>

typedef uint16_t SlabIndexType;

typedef struct {
    uint16_t      allocation_size;
    SlabIndexType num_entries;
    uint32_t      bitmap_size;
} SlabHeader;

void init_slab(ptr_t mem_start, ptr_t mem_end, size_t allocation_size);

ptr_t slab_alloc(SlabHeader* slab);
void slab_free(SlabHeader* slab, ptr_t memory);

size_t slab_overhead(SlabHeader* slab);

ptr_t slab_mem(SlabHeader* slab, SlabIndexType idx);
SlabIndexType slab_index(SlabHeader* slab, ptr_t memory);

#endif
