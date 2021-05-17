#ifndef _FLEXARRAY_H_INCLUDED
#define _FLEXARRAY_H_INCLUDED

#include <stdint.h>
#include <allocator.h>

struct flexarray;
typedef struct flexarray* flexarray_t;

flexarray_t new_flexarray(size_t member_size, size_t initial_alloc, allocator_t* alloc);
void delete_flexarray(flexarray_t array);

uint64_t flexarray_append(flexarray_t array, void* data);
void     flexarray_prepend(flexarray_t array, void* data);
void     flexarray_remove(flexarray_t array, uint64_t idx);

//! Will return -1 when not found
uint64_t flexarray_find(flexarray_t array, void* data);

size_t flexarray_length(flexarray_t array);
size_t flexarray_member_size(flexarray_t array);
void flexarray_get(flexarray_t array, void* buffer, uint64_t idx);
void flexarray_set(flexarray_t array, void* buffer, uint64_t idx);

const void* flexarray_getall(flexarray_t array);

#endif
