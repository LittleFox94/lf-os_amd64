#include <memory/context.h>

void MemoryContext::map(uint64_t vaddr, std::shared_ptr<MemoryObject> object) {
    auto prev = _objects.before_begin();
    for(auto it = _objects.begin(); it != _objects.end(); ++it) {
        if(it->vaddr > vaddr) {
            break;
        }

        prev = it;
    }

    _objects.emplace_after(prev, vaddr, object);
}

bool MemoryContext::handle_fault(uint64_t, MemoryFault&) {
    return false;
}
