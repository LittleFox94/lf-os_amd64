#ifndef _BITMAP_H_INCLUDED
#define _BITMAP_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t* bitmap_t;

static inline size_t bitmap_size(size_t num_entries) {
    return (num_entries + 7) / 8;
}

static inline uint64_t bitmap_idx(uint64_t entry) {
    return entry / 8;
}

static inline uint8_t bitmap_bit(uint64_t entry) {
    return 1 << (entry % 8);
}

static inline bool bitmap_get(bitmap_t bitmap, uint64_t entry) {
    return (bitmap[bitmap_idx(entry)] & bitmap_bit(entry)) != 0;
}

static inline void bitmap_set(bitmap_t bitmap, uint64_t entry) {
    bitmap[bitmap_idx(entry)] |= bitmap_bit(entry);
}

static inline void bitmap_clear(bitmap_t bitmap, uint64_t entry) {
    bitmap[bitmap_idx(entry)] &= ~bitmap_bit(entry);
}

#endif
