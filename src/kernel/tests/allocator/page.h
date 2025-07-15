#ifndef _ALLOCATOR_PAGE_H_INCLUDED
#define _ALLOCATOR_PAGE_H_INCLUDED

struct PageAllocator {
    template<
        class T,
        size_t PageSize
    >
    struct allocator : public AllocatorBase<T> {
        static_assert(sizeof(T) <= PageSize, "sizeof(T) is bigger than the configured PageSize!");

        using value_type = T;

        T* allocate(size_t n) {
            return reinterpret_cast<T*>(aligned_alloc(4096, n * PageSize));
        }

        void deallocate(T* p, size_t n) {
            free(reinterpret_cast<void*>(p));
        }

        template<typename U>
        struct rebind {
            typedef allocator<U, PageSize> other;
        };
    };
};

#endif
