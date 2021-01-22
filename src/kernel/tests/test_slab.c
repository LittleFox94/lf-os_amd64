#include <lfostest.h>
#include <slab.c>

__attribute__ ((visibility ("default"))) void testmain(TestUtils* t) {
    ptr_t memory = (ptr_t)malloc(1024 * 1024);
    init_slab(memory, memory + 1024*1024, 1024);

    SlabHeader* slab = (SlabHeader*)memory;
    ptr_t alloc1 = slab_alloc(slab);

    t->eq_ptr_t(memory + 1024, alloc1, "correct pointer returned");

    SlabIndexType idx2 = slab_index(slab, memory + 2048);
    t->eq_size_t(1,             idx2,   "correct index returned");

    free((void*)memory);
}
