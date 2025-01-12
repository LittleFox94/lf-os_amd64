#ifndef _ALLOCATOR_BASE_H_INCLUDED
#define _ALLOCATOR_BASE_H_INCLUDED

template<class T>
struct AllocatorBase {
    template<class... Args>
    void construct(T* p, Args&&... args) {
        ::new(static_cast<void*>(p)) T(args...);
    }

    template<class... Args>
    void destroy(T* p) {
        p->~T();
    }
};

#endif

