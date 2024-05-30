#include <lfostest.h>

namespace LFOS {
    #include <slab.cpp>

    TEST(KernelSlab, Simple) {
        const size_t size = 1024 * 1024;

        void* memory = malloc(size);
        init_slab((ptr_t)memory, (uint64_t)memory + size, 1024);

        SlabHeader* slab = (SlabHeader*)memory;
        ptr_t alloc1 = slab_alloc(slab);

        EXPECT_EQ(alloc1, (uint64_t)memory + 1024) << "correct pointer returned";

        SlabIndexType idx2 = slab_index(slab, (uint64_t)memory + 2048);
        EXPECT_EQ(idx2, 1) << "correct index returned";

        free((void*)memory);
    }
}
