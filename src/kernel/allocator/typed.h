#ifndef _ALLOCATOR_TYPED_H_INCLUDED
#define _ALLOCATOR_TYPED_H_INCLUDED

#include <allocator/base.h>
#include <allocator/sized.h>

template<
    class T,
    class Allocator = SizedAllocator<sizeof(T)>
>
struct TypedAllocator : public AllocatorBase<T> {
    private:
        Allocator _allocator;

    public:
        using value_type = T;

        T* allocate(size_t n) {
            return _allocator.template allocate<T>(n);
        }

        void deallocate(T* p, size_t n) {
            return _allocator.deallocate(reinterpret_cast<typename Allocator::value_type>(p), n);
        }

        template<typename U>
        struct rebind {
            typedef TypedAllocator<U> other;
        };
};

#endif
