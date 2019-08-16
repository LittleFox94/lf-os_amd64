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

static inline size_t slab_overhead(const SlabHeader* slab) {
    size_t overhead = sizeof(SlabHeader) + slab->bitmap_size;
    return overhead + (slab->allocation_size - (overhead % slab->allocation_size));
}

static inline ptr_t slab_mem(const SlabHeader* slab, const SlabIndexType idx) {
    return (idx * slab->allocation_size) + slab_overhead(slab) + (ptr_t)slab;
}

static inline SlabIndexType slab_index(const SlabHeader* slab, const ptr_t mem) {
    return (mem - slab_overhead(slab) - (ptr_t)slab) / slab->allocation_size;
}

#endif
