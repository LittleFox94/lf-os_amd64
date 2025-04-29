#ifndef _MEMORY_OBJECT_H_INCLUDED
#define _MEMORY_OBJECT_H_INCLUDED

#include <memory/types.h>

#include <memory>
#include <forward_list>

/**
 * A memory object is a piece of memory than can be mapped into userspace processes.
 *
 * In the most basic case, it's what is created for an mmap() syscall or
 * implicitly for each thread when creating a space for its stack.
 *
 * More complex uses of this include memory shared between multiple processes,
 * either like traditional shared memory for IPC purposes or reusing already
 * loaded shared objects.
 */
class MemoryObject {
    public:
        struct Mapping {
            /** virtual start address of this range relative to where the
             *  object is mapped in a context. Must be aligned to the smallest
             *  page size on the target platform.
             */
            uint64_t vaddr_start;

            /** physical start address for this range. Might be 0, which is
             *  useful to have this object handle non-present faults. Must be
             *  aligned to the smallest page size on the target plaform.
             */
            uint64_t paddr_start;

            /** Size of this range in bytes. Must be a multiple of the smallest
             *  page size on the target platform.
             */
            size_t length;

            //! What can this range be used for?
            MemoryAccess usage;
        };

        virtual ~MemoryObject() { }

        /** Creates a copy of this object, potentially modifying this object (to
         *  allow for e.g. Copy-on-Write) but it might also just return this
         *  object without changing anything on it (for e.g. readonly objects).
         *
         *  The copy might be of the same type or a completely different type -
         *  who knows? :o)
         */
        virtual std::shared_ptr<MemoryObject> copy() = 0;

        virtual std::forward_list<Mapping> getMappings() = 0;
};

#endif
