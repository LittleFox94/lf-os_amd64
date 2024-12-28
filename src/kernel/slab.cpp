#include "slab.h"
#include "bitmap.h"
#include "string.h"
#include "panic.h"

void init_slab(uint64_t mem_start, uint64_t mem_end, size_t allocation_size) {
    size_t mem_total   = mem_end   - mem_start;
    size_t num_entries = mem_total / allocation_size;
    size_t overhead    = bitmap_size(num_entries) + sizeof(SlabHeader);

    num_entries -= (overhead + allocation_size) / allocation_size;

    SlabHeader* header = (SlabHeader*)mem_start;
    header->allocation_size = allocation_size;
    header->num_entries     = num_entries;
    header->bitmap_size     = bitmap_size(num_entries);

    bitmap_t bitmap = (bitmap_t)((uint64_t)header + sizeof(SlabHeader));
    memset(bitmap, 0, header->bitmap_size);
}

uint64_t slab_alloc(SlabHeader* slab) {
    bitmap_t bitmap = (bitmap_t)((uint64_t)slab + sizeof(SlabHeader));

    SlabIndexType idx;
    for(idx = 0; idx < slab->num_entries && bitmap_get(bitmap, idx); ++idx);

    if(idx < slab->num_entries) {
        bitmap_set(bitmap, idx);
        return slab_mem(slab, idx);
    }

    panic_message("Out of entries in slab allocator");
    return 0;
}

void slab_free(SlabHeader* slab, uint64_t mem) {
    bitmap_t bitmap = (bitmap_t)((uint64_t)slab + sizeof(SlabHeader));
    bitmap_clear(bitmap, slab_index(slab, mem));
}

