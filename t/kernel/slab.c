#include <lfostest.h>

#include "../../src/kernel/slab.c"

int main() {
    bool error = false;

    ptr_t memory = (ptr_t)malloc(1024 * 1024);
    init_slab(memory, memory + 1024*1024, 1024);

    SlabHeader* slab = (SlabHeader*)memory;
    ptr_t alloc1 = slab_alloc(slab);

    eq(memory + 1024, alloc1, "correct pointer returned");

    SlabIndexType idx2 = slab_index(slab, memory + 2048);
    eq(1,             idx2,   "correct index returned");

    return error ? 1 : 0;
}
