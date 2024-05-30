#include <uuid.h>
#include <string.h>

uuid_key_t uuid_key(const uuid_t* uuid) {
    uuid_key_t key = 0;

    for(uint8_t i = 0; i < sizeof(uuid->data); ++i) {
        key ^= uuid->data[i];
    }

    return key;
}

int uuid_cmp(const uuid_t* a, const uuid_t* b) {
    return memcmp(a->data, b->data, sizeof(a->data));
}

size_t uuid_fmt(char* buffer, size_t len, const uuid_t* uuid) {
    return ksnprintf(buffer, len,
        "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
        (uint64_t)uuid->uuid_members.a,    (uint64_t)uuid->uuid_members.b,    (uint64_t)uuid->uuid_members.c, (uint64_t)uuid->uuid_members.d,
        (uint64_t)uuid->uuid_members.e[0], (uint64_t)uuid->uuid_members.e[1], (uint64_t)uuid->uuid_members.e[2],
        (uint64_t)uuid->uuid_members.e[3], (uint64_t)uuid->uuid_members.e[4], (uint64_t)uuid->uuid_members.e[5]
    );
}
