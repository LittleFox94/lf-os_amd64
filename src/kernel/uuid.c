#include <uuid.h>

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

int uuid_fmt(char* buffer, size_t len, uuid_t* uuid) {
    int ret = ksnprintf(buffer, len,
        "%08x-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
        uuid->a, uuid->b, uuid->c, uuid->d,
        uuid->e[0], uuid->e[2], uuid->e[2],
        uuid->e[3], uuid->e[4], uuid->e[5]
    );

    if(ret < len) {
        buffer[ret++] = 0;
    }

    return ret;
}
