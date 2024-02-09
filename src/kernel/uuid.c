#include <uuid.h>
#include <string.h>

uuid_key_t uuid_key(uuid_t* uuid) {
    uuid_key_t key = 0;

    for(uint8_t i = 0; i < sizeof(uuid->data); ++i) {
        key ^= uuid->data[i];
    }

    return key;
}

int uuid_cmp(uuid_t* a, uuid_t* b) {
    return memcmp(a->data, b->data, sizeof(a->data));
}

size_t uuid_fmt(char* buffer, size_t len, uuid_t* uuid) {
    return ksnprintf(buffer, len,
        "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
        (uint64_t)uuid->a,    (uint64_t)uuid->b,    (uint64_t)uuid->c, (uint64_t)uuid->d,
        (uint64_t)uuid->e[0], (uint64_t)uuid->e[1], (uint64_t)uuid->e[2],
        (uint64_t)uuid->e[3], (uint64_t)uuid->e[4], (uint64_t)uuid->e[5]
    );
}
