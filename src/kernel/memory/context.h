#ifndef _MEMORY_CONTEXT_H_INCLUDED
#define _MEMORY_CONTEXT_H_INCLUDED

#include <memory>
#include <forward_list>

#include <memory/types.h>
#include <memory/object.h>

/**
 * A memory context is an abstraction for a virtual memory context, from which
 * architecture-specific paging data structures are generated.
 *
 * Memory is mapped into a context using memory objects - see memory/objects.h
 * for more information on them.
 */
class MemoryContext {
    public:
        MemoryContext() {
        }

        void map(uint64_t vaddr, std::shared_ptr<MemoryObject> object);

        bool handle_fault(uint64_t, MemoryFault&);

        /** Returns a pointer to the architecture-specific paging structures
         *  generated from this generic context.
         *
         *  To be implemented in architecture-specific code, generating the
         *  architecture specific paging structures (either on the fly or, in
         *  the best case, caching them using the _arch_cache member of this
         *  class).
         */
        void* arch_context();

    private:
        struct MappedObject {
            uint64_t vaddr;
            std::shared_ptr<MemoryObject> object;

            MappedObject(uint64_t vaddr, std::shared_ptr<MemoryObject> object) :
                vaddr(vaddr), object(object) {
            }

            MappedObject(MappedObject& other) :
                vaddr(other.vaddr), object(other.object->copy()) {
            }
        };

        std::forward_list<MappedObject> _objects;

        void* _arch_cache;
};

#endif
