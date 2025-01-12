#ifndef _ALLOCATOR_PAGE_H_INCLUDED
#define _ALLOCATOR_PAGE_H_INCLUDED

#include "base.h"

#include <stddef.h>

#include <mm.h>
#include <vm.h>
#include <log.h>

struct PageAllocator {
    template<
        class T,
        size_t PageSize
    >
    struct allocator : public AllocatorBase<T> {
        static_assert(sizeof(T) <= PageSize, "sizeof(T) is bigger than the configured PageSize!");

        typedef T value_type;
        static const size_t page_count = PageSize / 4096;

        T* allocate(size_t n) {
            if(n * sizeof(T) > PageSize) {
                logf("PageAllocator", "Tried to allocate above PageSize (%d > %d)!", n * sizeof(T), PageSize);
                return 0;
            }

            logd("PageAllocator", "Allocating page ..");
            return reinterpret_cast<T*>(vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, page_count * n));
        }

        void deallocate(T* p, size_t n) {
            logd("PageAllocator", "Freeing page ..");

            uint64_t vir = reinterpret_cast<uint64_t>(p);
            for(size_t i = 0; i < page_count * n; ++i) {
                uint64_t phy = vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, vir);
                vm_context_unmap(VM_KERNEL_CONTEXT, vir + (i * 4096));
                mm_mark_physical_pages(phy, 1, MM_FREE);
            }
        }

        template<typename U>
        struct rebind {
            typedef allocator<U, PageSize> other;
        };
    };
};

#endif
