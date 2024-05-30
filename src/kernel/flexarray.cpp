#include <flexarray.h>
#include <string.h>

struct flexarray {
    size_t member_size;
    size_t count;
    size_t alloc;

    allocator_t*   allocator;

    void* data;
};

flexarray_t new_flexarray(size_t member_size, size_t initial_alloc, allocator_t* alloc) {
    if(!initial_alloc) {
        initial_alloc = 8;
    }

    flexarray_t res  = (flexarray_t)alloc->alloc(alloc, sizeof(struct flexarray));
    res->member_size = member_size;
    res->alloc       = initial_alloc;
    res->count       = 0;
    res->allocator   = alloc;
    res->data        = alloc->alloc(alloc, initial_alloc * member_size);

    return res;
}

void delete_flexarray(flexarray_t array) {
    array->allocator->dealloc(array->allocator, array);
}

static void flexarray_grow(flexarray_t array, size_t copy_offset) {
    size_t grow = array->alloc >> 4;

    if(!grow) {
        grow = 1;
    }

    size_t old_size  = array->alloc * array->member_size;
    size_t new_alloc = array->alloc + grow;
    size_t new_size  = new_alloc * array->member_size;

    void* new_data = array->allocator->alloc(array->allocator, new_size);
    memcpy(new_data, (uint8_t*)array->data + (copy_offset * array->member_size), old_size);
    array->allocator->dealloc(array->allocator, array->data);

    array->data  = new_data;
    array->alloc = new_alloc;
}

static void* flexarray_data(flexarray_t array, uint64_t idx) {
    return (uint8_t*)array->data + (idx * array->member_size);
}

uint64_t flexarray_append(flexarray_t array, void* data) {
    if(array->count >= array->alloc) {
        flexarray_grow(array, 0);
    }

    memcpy(flexarray_data(array, array->count), data, array->member_size);
    ++array->count;

    return array->count - 1;
}

void flexarray_prepend(flexarray_t array, void* data) {
    if(array->count >= array->alloc) {
        flexarray_grow(array, 1);
    }
    else {
        memmove(flexarray_data(array, 1), array->data, array->count * array->member_size);
    }

    memcpy(flexarray_data(array, 0), data, array->member_size);
}

void flexarray_remove(flexarray_t array, uint64_t idx) {
    if(idx >= array->count) {
        return;
    }

    --array->count;

    if(idx >= array->count) {
        return;
    }

    memmove(flexarray_data(array, idx + 1), flexarray_data(array, idx), (array->count - idx) * array->member_size);
}

uint64_t flexarray_find(flexarray_t array, void* data) {
    for(uint64_t i = 0; i < array->count; ++i) {
        if(memcmp(flexarray_data(array, i), data, array->member_size) == 0) {
            return i;
        }
    }

    return -1;
}

size_t flexarray_length(flexarray_t array) {
    return array->count;
}

size_t flexarray_member_size(flexarray_t array) {
    return array->member_size;
}

void flexarray_get(flexarray_t array, void* buffer, uint64_t idx) {
    if(idx >= array->count) {
        return;
    }

    memcpy(buffer, flexarray_data(array, idx), array->member_size);
}

void flexarray_set(flexarray_t array, void* buffer, uint64_t idx) {
    if(idx >= array->count) {
        return;
    }

    memcpy(flexarray_data(array, idx), buffer, array->member_size);
}

const void* flexarray_getall(flexarray_t array) {
    return flexarray_data(array, 0);
}
