#include <memory/plain_object.h>

#include <mm.h>
#include <vm.h>
#include <string.h>
#include <log.h>

PlainMemoryObject::PlainMemoryObject(size_t size, uint8_t* data, size_t data_len, size_t offset_from_start)
    : PlainMemoryObject(size) {

    logd("PlainMemoryObject", "Constructing from existing data of %llu bytes at 0x%x", data_len, data);

    for(auto page : _pages) {
        if(offset_from_start >= 4096) {
            offset_from_start -= 4096;
            continue;
        }

        if (data_len > 0) {
            auto to_copy = 4096 - offset_from_start;

            if(to_copy > data_len) {
                to_copy = data_len;
            }

            memcpy(reinterpret_cast<void*>(offset_from_start + page + ALLOCATOR_REGION_DIRECT_MAPPING.start), data, to_copy);

            data += to_copy;
            data_len -= to_copy;
            offset_from_start = 0;
        }
    }
}

PlainMemoryObject::PlainMemoryObject(size_t size) {
    logd("PlainMemoryObject", "Constructing %llu zero bytes", size);

    auto it = _pages.before_begin();
    for(size_t i = 0; i < size; i+= 4096) {
        uint8_t* page = static_cast<uint8_t*>(mm_alloc_pages(1));
        it = _pages.emplace_after(it, (uint64_t)page);
        memset(page + ALLOCATOR_REGION_DIRECT_MAPPING.start, 0, 4096);
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
        mapping_it = ret.emplace_after(mapping_it, vaddr, *page_it, 4096, MemoryAccess::Execute);
        vaddr += 4096;
    }

    return ret;
}
