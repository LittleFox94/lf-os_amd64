#include <memory/plain_object.h>

#include <mm.h>
#include <vm.h>
#include <string.h>
#include <log.h>

PlainMemoryObject::PlainMemoryObject(uint8_t* data, size_t len) {
    logd("PlainMemoryObject", "Constructing from existing data of %llu bytes at 0x%x", len, data);

    auto it = _pages.before_begin();
    for(size_t i = 0; i < len; i+= 4096) {
        size_t this_len = len - i;
        if(this_len > 4096) {
            this_len = 4096;
        }

        uint8_t* page = static_cast<uint8_t*>(mm_alloc_pages(1));
        it = _pages.emplace_after(it, (uint64_t)page);
        memcpy(page + ALLOCATOR_REGION_DIRECT_MAPPING.start, data + i, this_len);
    }
}

std::shared_ptr<MemoryObject> PlainMemoryObject::copy() {
    // TODO: implement
    return std::shared_ptr<MemoryObject>(this);
}

std::forward_list<MemoryObject::Mapping> PlainMemoryObject::getMappings() {
    std::forward_list<MemoryObject::Mapping> ret;

    uint64_t vaddr = 0;
    auto mapping_it = ret.before_begin();
    for(auto page_it = _pages.begin(); page_it != _pages.end(); ++page_it) {
        ret.emplace_after(mapping_it, vaddr, *page_it, 4096, MemoryAccess::Execute);
    }

    return ret;
}
