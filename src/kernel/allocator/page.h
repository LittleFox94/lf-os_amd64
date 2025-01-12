#ifndef _ALLOCATOR_PAGE_H_INCLUDED
#define _ALLOCATOR_PAGE_H_INCLUDED

#include "base.h"

#include <stddef.h>
#include <bitset>

#include <mm.h>
#include <vm.h>
#include <log.h>

#include "../bitset_helpers.h"

struct PageAllocatorBase {
    public:
        static bool memory_management_bootstrapped;

    protected:
        template<size_t PageSize>
        struct helpers {
            //! Tracks the number of calls to allocate_pages
            static size_t allocations;
            //! Tracks the number of calls to deallocate_pages
            static size_t deallocations;
            //! Tracks the number of pages allocated right now (allocate_pages(2) will increment this by two)
            static size_t allocated;

            struct alignas(PageSize) value_type {
                uint8_t data[PageSize];
            }__attribute__((packed));

            private:
                static const size_t page_count = PageSize / 4096;
                static const size_t num_bootstrap_pages = PageSize == 4096 ? 16 : 0;

                static value_type                       bootstrap_pages[num_bootstrap_pages];
                static std::bitset<num_bootstrap_pages> bootstrap_page_status;

                static value_type* allocate_bootstrap_pages(size_t n) {
                    size_t start = bitset_helpers<num_bootstrap_pages>::find_continuous_unset(bootstrap_page_status, n);
                    if(start >= num_bootstrap_pages) {
                        logf("PageAllocator/allocate_bootstrap_pages", "No bootstrap pages left for PageSize = %u!", PageSize);
                        return 0;
                    }

                    bitset_helpers<num_bootstrap_pages>::set_range(bootstrap_page_status, start, n);
                    return &bootstrap_pages[start];
                }

                static void deallocate_bootstrap_pages(value_type*, size_t) {
                }

            public:
                static value_type* allocate_pages(size_t n) {
                    ++allocations;
                    allocated += n;

                    if(!memory_management_bootstrapped) {
                        return allocate_bootstrap_pages(n);
                    }

                    return reinterpret_cast<value_type*>(vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, page_count * n));
                }

                static void deallocate_pages(value_type* p, size_t n) {
                    ++deallocations;
                    allocated -= n;

                    if(p >= &bootstrap_pages[0] && p < &bootstrap_pages[num_bootstrap_pages]) {
                        return deallocate_bootstrap_pages(p, n);
                    }


                    uint64_t vir = reinterpret_cast<uint64_t>(p);
                    for(size_t i = 0; i < page_count * n; ++i) {
                        uint64_t phy = vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, vir);
                        vm_context_unmap(VM_KERNEL_CONTEXT, vir + (i * 4096));
                        mm_mark_physical_pages(phy, 1, MM_FREE);
                    }
                }
        };
};

template<size_t PageSize>
size_t PageAllocatorBase::helpers<PageSize>::allocations = 0;

template<size_t PageSize>
size_t PageAllocatorBase::helpers<PageSize>::deallocations = 0;

template<size_t PageSize>
size_t PageAllocatorBase::helpers<PageSize>::allocated = 0;

template<size_t PageSize>
PageAllocatorBase::helpers<PageSize>::value_type PageAllocatorBase::helpers<PageSize>::bootstrap_pages[];

template<size_t PageSize>
std::bitset<PageAllocatorBase::helpers<PageSize>::num_bootstrap_pages> PageAllocatorBase::helpers<PageSize>::bootstrap_page_status;

struct PageAllocator {
    template<
        class T,
        size_t PageSize
    >
    struct allocator : public AllocatorBase<T>, protected PageAllocatorBase {
        static_assert(sizeof(T) <= PageSize, "sizeof(T) is bigger than the configured PageSize!");

        using value_type = T;
        using helper_type  = PageAllocatorBase::helpers<PageSize>;

        T* allocate(size_t n) {
            return reinterpret_cast<T*>(helper_type::allocate_pages(n));
        }

        void deallocate(T* p, size_t n) {
            helper_type::deallocate_pages(reinterpret_cast<helper_type::value_type*>(p), n);
        }

        template<typename U>
        struct rebind {
            typedef allocator<U, PageSize> other;
        };
    };
};

#endif
