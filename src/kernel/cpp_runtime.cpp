#include <panic.h>
#include <allocator/sized.h>

extern "C" void __cxa_pure_virtual() {
    panic_message("Voi ei, a pure-virtual method was called!");
}

static SizedAllocator<8> sa8;
static SizedAllocator<16> sa16;
static SizedAllocator<24> sa24;
static SizedAllocator<64> sa64;
static SizedAllocator<512> sa512;
static SizedAllocator<1024> sa1024;
static SizedAllocator<4096> sa4096;
static SizedAllocator<16384> sa16384;
static SizedAllocator<65536> sa65536;

static SizedAllocatorBase* allocators[] = {
    &sa8,
    &sa16,
    &sa24,
    &sa64,
    &sa512,
    &sa1024,
    &sa4096,
    &sa16384,
    &sa65536
};

static SizedAllocatorBase* get_allocator(unsigned long len) {
    for(size_t i = 0; i < sizeof(allocators) / sizeof(allocators[0]); ++i) {
        if(allocators[i]->size() >= len) {
            return allocators[i];
        }
    }

    return 0;
}

void* operator new(unsigned long len) {
    auto alloc = get_allocator(len);
    if(!alloc) {
        #pragma clang diagnostic ignored "-Wnew-returns-null"
        #pragma clang diagnostic ignored "-Wnonnull"
        return 0;
    }
    return alloc->allocate(1);
}

void operator delete(void* ptr, unsigned long len) {
    auto alloc = get_allocator(len);
    if(alloc) {
        return alloc->deallocate(ptr, 1);
    }
}
