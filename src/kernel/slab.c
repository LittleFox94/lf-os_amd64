#include "slab.h"
#include "bitmap.h"

void init_slab(ptr_t mem_start, ptr_t mem_end, size_t allocation_size) {
    size_t mem_total   = mem_end   - mem_start;
    size_t num_entries = mem_total / allocation_size;
    size_t overhead    = bitmap_size(num_entries) + sizeof(SlabHeader);

    num_entries -= (overhead + allocation_size) / allocation_size;

    SlabHeader* header = (SlabHeader*)mem_start;
    header->allocation_size = allocation_size;
    header->num_entries     = num_entries;
    header->bitmap_size     = bitmap_size(num_entries);

    for(size_t i = 0; i < num_entries; ++i) {
        slab_free(header, mem_start + overhead + i);
    }
}

ptr_t slab_alloc(SlabHeader* slab) {
    bitmap_t bitmap = (bitmap_t)((ptr_t)slab + sizeof(SlabHeader));

    SlabIndexType idx;
    for(idx = 0; idx < slab->num_entries && bitmap_get(bitmap, idx); ++idx);

    if(idx < slab->num_entries) {
        bitmap_set(bitmap, idx);
        return slab_mem(slab, idx);
    }

    // XXX: crash, slab allocator out of free entries
    while(1);
}

void slab_free(SlabHeader* slab, ptr_t mem) {
    bitmap_t bitmap = (bitmap_t)((ptr_t)slab + sizeof(SlabHeader));

    for(SlabIndexType i = 0; i < slab->num_entries; ++i) {
        bitmap_clear(bitmap, i);
    }
}

size_t slab_overhead(SlabHeader* slab) {
    size_t overhead = sizeof(SlabHeader) + slab->bitmap_size;
    return overhead + (slab->allocation_size - (overhead % slab->allocation_size));
}

ptr_t slab_mem(SlabHeader* slab, SlabIndexType idx) {
    return (idx * slab->allocation_size) + slab_overhead(slab) + (ptr_t)slab;
}

SlabIndexType slab_index(SlabHeader* slab, ptr_t mem) {
    return (mem - slab_overhead(slab) - (ptr_t)slab) / slab->allocation_size;
}
