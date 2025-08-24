#ifndef _MEMORY_PLAIN_OBJECT_H_INCLUDED
#define _MEMORY_PLAIN_OBJECT_H_INCLUDED

#include <memory/object.h>

class PlainMemoryObject : public MemoryObject {
    public:
        PlainMemoryObject(size_t size);
        PlainMemoryObject(size_t size, uint8_t* data, size_t data_len, size_t offset_from_start = 0);
        PlainMemoryObject(uint8_t* data, size_t data_len)
            : PlainMemoryObject(data_len, data, data_len, 0) { }

        virtual std::shared_ptr<MemoryObject> copy() override;

        virtual std::forward_list<Mapping> getMappings() override;

    private:
        std::forward_list<uint64_t> _pages;
};

#endif
