#ifndef _MEMORY_PLAIN_OBJECT_H_INCLUDED
#define _MEMORY_PLAIN_OBJECT_H_INCLUDED

#include <memory/object.h>

class PlainMemoryObject : public MemoryObject {
    public:
        PlainMemoryObject(uint8_t* data, size_t len);

        virtual std::shared_ptr<MemoryObject> copy() override;

        virtual std::forward_list<Mapping> getMappings() override;

    private:
        std::forward_list<uint64_t> _pages;
};

#endif
