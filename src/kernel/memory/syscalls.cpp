#include <allocator.h>
#include <scheduler.h>

#include <errno.h>

#include <memory/object.h>
#include <memory/plain_object.h>
#include <memory/context.h>

#include <tpa.h>

static TPA<typename std::shared_ptr<PlainMemoryObject>>* memory_objects = 0;
static uint64_t next_memory_object = 1;

static void ensure_memory_objects_initialized() {
    if (memory_objects) {
        return;
    }

    memory_objects = TPA<typename std::shared_ptr<PlainMemoryObject>>::create(&kernel_alloc, 4080, 0);
}

void sc_handle_memory_create_memory_object(uint64_t size, uint64_t source, uint64_t flags, uint64_t* error, uint64_t* memory_object_handle) {
    ensure_memory_objects_initialized();

    if(source != 0 || flags != 0) {
        *error = ENOSYS;
        *memory_object_handle = 0;
        return;
    }

    *memory_object_handle = next_memory_object++;

    auto ptr = std::make_shared<PlainMemoryObject>(size);
    memory_objects->set(*memory_object_handle, &ptr);
    *error = 0;
}

void sc_handle_memory_map(uint64_t memory_object_handle, void* vaddr, uint64_t flags, uint64_t* error, void** mapped_address, size_t* length) {
    if(flags != 0 || vaddr == nullptr) {
        *error = ENOSYS;
        return;
    }

    auto memory_object_ptr = memory_objects->get(memory_object_handle);
    if(!memory_object_ptr) {
        *error = ENOENT;
        return;
    }

    auto memory_object = *memory_object_ptr;
    auto context = process_context(scheduler_current_process);
    if(!context) {
        *error = EINVAL; // shouldn't be possible
        return;
    }

    context.value()->map(reinterpret_cast<uint64_t>(vaddr), std::static_pointer_cast<MemoryObject>(memory_object));

    *error = 0;
    *mapped_address = vaddr;
    *length = 0;
}
