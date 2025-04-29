#ifndef _MEMORY_TYPES_H_INCLUDED
#define _MEMORY_TYPES_H_INCLUDED

enum class MemoryAccess {
    Read,
    Write,
    Execute,
};

struct MemoryFault {
    MemoryAccess access;
    bool         present;
};

#endif
