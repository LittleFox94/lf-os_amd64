#ifndef _SYS_UUID_H_INCLUDE
#define _SYS_UUID_H_INCLUDE

#include <stdint.h>

typedef union {
    struct {
        uint32_t a;
        uint16_t b;
        uint16_t c;
        uint16_t d;
        uint8_t  e[6];
    };

    uint8_t data[16];
} uuid_t;

#if __kernel
//! Type for fast UUID lookups, not unique!
typedef uint8_t uuid_key_t;

uuid_key_t uuid_key(uuid_t* uuid);
int uuid_cmp(uuid_t* a, uuid_t* b);
size_t uuid_fmt(char* buffer, size_t len, uuid_t* uuid);
#endif

#endif
