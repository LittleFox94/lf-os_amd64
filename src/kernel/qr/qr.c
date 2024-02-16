#include <qr.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <log.h>
#include <bitmap.h>

void qr_block_ec_generate(uint8_t* block, size_t block_size, uint8_t* ec_result, size_t num_ec_words);

struct qr_version_data {
    uint32_t version_info;
    uint8_t  ec_codewords_per_block;
    uint8_t* block_sizes;
    uint8_t* alignment_locations;
};

static struct qr_version_data qr_versions[] = {
    { 0 }, // just for padding to have QR version = index in this array

    {       0,  7, (uint8_t[]){  1,  19,          0 }, (uint8_t[]){ 0 } },
    {       0, 10, (uint8_t[]){  1,  34,          0 }, (uint8_t[]){ 6, 18, 0 } },
    {       0, 15, (uint8_t[]){  1,  55,          0 }, (uint8_t[]){ 6, 22, 0 } },
    {       0, 20, (uint8_t[]){  1,  80,          0 }, (uint8_t[]){ 6, 26, 0 } },
    {       0, 26, (uint8_t[]){  1, 108,          0 }, (uint8_t[]){ 6, 30, 0 } },
    {       0, 18, (uint8_t[]){  2,  68,          0 }, (uint8_t[]){ 6, 34, 0 } },
    {  0x7C94, 20, (uint8_t[]){  2,  78,          0 }, (uint8_t[]){ 6, 22, 38, 0 } },
    {  0x85BC, 24, (uint8_t[]){  2,  97,          0 }, (uint8_t[]){ 6, 24, 42, 0 } },
    {  0x9A99, 30, (uint8_t[]){  2, 116,          0 }, (uint8_t[]){ 6, 26, 46, 0 } },
    {  0xA4D3, 18, (uint8_t[]){  2,  68,  2,  69, 0 }, (uint8_t[]){ 6, 28, 50, 0 } },
    {  0xBBF6, 20, (uint8_t[]){  4,  81,          0 }, (uint8_t[]){ 6, 30, 54, 0 } },
    {  0xC762, 24, (uint8_t[]){  2,  92,  2,  93, 0 }, (uint8_t[]){ 6, 32, 58, 0 } },
    {  0xD847, 26, (uint8_t[]){  4, 107,          0 }, (uint8_t[]){ 6, 34, 62, 0 } },
    {  0xE60D, 30, (uint8_t[]){  3, 115,  1, 116, 0 }, (uint8_t[]){ 6, 26, 46, 66, 0 } },
    {  0xF928, 30, (uint8_t[]){  5,  87,  1,  88, 0 }, (uint8_t[]){ 6, 26, 48, 70, 0 } },
    { 0x10B78, 22, (uint8_t[]){  5,  98,  1,  99, 0 }, (uint8_t[]){ 6, 26, 50, 74, 0 } },
    { 0x1145D, 24, (uint8_t[]){  1, 107,  5, 108, 0 }, (uint8_t[]){ 6, 30, 54, 78, 0 } },
    { 0x12A17, 28, (uint8_t[]){  5, 120,  1, 121, 0 }, (uint8_t[]){ 6, 30, 56, 82, 0 } },
    { 0x13532, 30, (uint8_t[]){  3, 113,  4, 114, 0 }, (uint8_t[]){ 6, 30, 58, 86, 0 } },
    { 0x149A6, 28, (uint8_t[]){  3, 107,  5, 108, 0 }, (uint8_t[]){ 6, 34, 62, 90, 0 } },
    { 0x15683, 28, (uint8_t[]){  4, 116,  4, 117, 0 }, (uint8_t[]){ 6, 28, 50, 72, 94, 0 } },
    { 0x168C9, 28, (uint8_t[]){  2, 111,  7, 112, 0 }, (uint8_t[]){ 6, 26, 50, 74, 98, 0 } },
    { 0x177EC, 30, (uint8_t[]){  4, 121,  5, 122, 0 }, (uint8_t[]){ 6, 30, 54, 78, 102, 0 } },
    { 0x18EC4, 30, (uint8_t[]){  6, 117,  4, 118, 0 }, (uint8_t[]){ 6, 28, 54, 80, 106, 0 } },
    { 0x191E1, 26, (uint8_t[]){  8, 106,  4, 107, 0 }, (uint8_t[]){ 6, 32, 58, 84, 110, 0 } },
    { 0x1AFAB, 28, (uint8_t[]){ 10, 114,  2, 115, 0 }, (uint8_t[]){ 6, 30, 58, 86, 114, 0 } },
    { 0x1B08E, 30, (uint8_t[]){  8, 122,  4, 123, 0 }, (uint8_t[]){ 6, 34, 62, 90, 118, 0 } },
    { 0x1CC1A, 30, (uint8_t[]){  3, 117, 10, 118, 0 }, (uint8_t[]){ 6, 26, 50, 74, 98, 122, 0 } },
    { 0x1D33F, 30, (uint8_t[]){  7, 116,  7, 117, 0 }, (uint8_t[]){ 6, 30, 54, 78, 102, 126, 0 } },
    { 0x1ED75, 30, (uint8_t[]){  5, 115, 10, 116, 0 }, (uint8_t[]){ 6, 26, 52, 78, 104, 130, 0 } },
    { 0x1F250, 30, (uint8_t[]){ 13, 115,  3, 116, 0 }, (uint8_t[]){ 6, 30, 56, 82, 108, 134, 0 } },
    { 0x209D5, 30, (uint8_t[]){ 17, 115,          0 }, (uint8_t[]){ 6, 34, 60, 86, 112, 138, 0 } },
    { 0x216F0, 30, (uint8_t[]){ 17, 115,  1, 116, 0 }, (uint8_t[]){ 6, 30, 58, 86, 114, 142, 0 } },
    { 0x228BA, 30, (uint8_t[]){ 13, 115,  6, 116, 0 }, (uint8_t[]){ 6, 34, 62, 90, 118, 146, 0 } },
    { 0x2379F, 30, (uint8_t[]){ 12, 121,  7, 122, 0 }, (uint8_t[]){ 6, 30, 54, 78, 102, 126, 150, 0 } },
    { 0x24B0B, 30, (uint8_t[]){  6, 121, 14, 122, 0 }, (uint8_t[]){ 6, 24, 50, 76, 102, 128, 154, 0 } },
    { 0x2542E, 30, (uint8_t[]){ 17, 122,  4, 123, 0 }, (uint8_t[]){ 6, 28, 54, 80, 106, 132, 158, 0 } },
    { 0x26A64, 30, (uint8_t[]){  4, 122, 18, 123, 0 }, (uint8_t[]){ 6, 32, 58, 84, 110, 136, 162, 0 } },
    { 0x27541, 30, (uint8_t[]){ 20, 117,  4, 118, 0 }, (uint8_t[]){ 6, 26, 54, 82, 110, 138, 166, 0 } },
    { 0x28C69, 30, (uint8_t[]){ 19, 118,  6, 119, 0 }, (uint8_t[]){ 6, 30, 58, 86, 114, 142, 170, 0 } },
};

static uint16_t qr_data_codewords(uint8_t version) {
    size_t ret = 0;
    uint8_t* block_size = qr_versions[version].block_sizes;
    while(*block_size) {
        ret += block_size[0] * block_size[1];
        block_size += 2;
    }

    return ret;
}

static uint16_t qr_all_codewords(uint8_t version) {
    size_t ret = 0;
    uint8_t* block_size = qr_versions[version].block_sizes;
    while(*block_size) {
        ret += block_size[0] * block_size[1];
        ret += block_size[0] * qr_versions[version].ec_codewords_per_block;
        block_size += 2;
    }

    return ret;
}

static uint16_t qr_userdata(uint8_t version) {
    //   mode indicator (4 bits)
    // + data length (8 or 16 bits)
    // + terminator (4 bits)
    // = 2 or 3 bytes
    uint8_t overhead = version <= 9 ? 2 : 3;

    return qr_data_codewords(version) + overhead;
}

static uint8_t qr_version(size_t data_len) {
    for(size_t i = 1; i < sizeof(qr_versions) / sizeof(struct qr_version_data); ++i) {
        // > and not >= for the mode indicator nibble
        if(qr_userdata(i) > data_len) {
            return i;
        }
    }

    return 0;
}

static uint8_t qr_modules(uint8_t version) {
    return 17 + (version * 4);
}

inline static bool qr_reserved(qr_data out, uint8_t x, uint8_t y) {
    return (out[(y * 177) + x] & 2) != 0;
}

inline static void qr_flip(qr_data out, uint8_t x, uint8_t y) {
    out[(y * 177) + x] ^= 1;
}

inline static void qr_set(qr_data out, uint8_t x, uint8_t y, bool safe) {
    uint8_t v = 1;

    if(safe && out[(y * 177) + x]) {
        v = 1 | 4;
    }

    out[(y * 177) + x] |= v;
}

inline static void qr_reserve(qr_data out, uint8_t x, uint8_t y) {
    out[(y * 177) + x] |= 2;
}

static void qr_finder(qr_data out, uint8_t x, uint8_t y) {
    for(uint8_t _x = x; _x < x+7; ++_x) {
        for(uint8_t _y = y; _y < y+7; ++_y) {
            qr_reserve(out, _x, _y);
        }
    }

    for(uint8_t i = 0; i < 8; ++i) {
        qr_reserve(out, x ? x-1 : x+7   , y+i+(y ? -1 : 0));
        qr_reserve(out, x+i+(x ? -1 : 0), y ? y-1 : y+7);
    }

    for(uint8_t _x = x; _x < x+7; ++_x) {
        qr_set(out, _x, y, false);
        qr_set(out, _x, y+6, false);
    }

    for(uint8_t _y = y; _y < y+7; ++_y) {
        qr_set(out, x, _y, false);
        qr_set(out, x+6, _y, false);
    }

    for(uint8_t _y = y+2; _y < y+5; ++_y) {
        for(uint8_t _x = x+2; _x < x+5; ++_x) {
            qr_set(out, _x, _y, false);
        }
    }
}

static void qr_alignment(qr_data out, uint8_t x, uint8_t y) {
    for(uint8_t _x = x-2; _x < x+3; ++_x) {
        for(uint8_t _y = y-2; _y < y+3; ++_y) {
            qr_reserve(out, _x, _y);
        }
    }

    qr_set(out, x, y, false);

    for(uint8_t _x = x-2; _x < x+3; ++_x) {
        qr_set(out, _x, y-2, false);
        qr_set(out, _x, y+2, false);
    }

    for(uint8_t _y = y-2; _y < y+3; ++_y) {
        qr_set(out, x-2, _y, false);
        qr_set(out, x+2, _y, false);
    }
}

static void qr_format_and_version(qr_data out, uint8_t version, uint8_t masking_pattern) {
    if(masking_pattern != 0) {
        loge("qr", "Unimplemented masking pattern %u", (unsigned)masking_pattern);
    }

    uint8_t modules = qr_modules(version);

    // hard-coded ECC level L and mask pattern 0
    uint8_t format[] = { 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0 };
    for(size_t i = 0; i < sizeof(format); ++i) {
        uint8_t lx = i < 8 ? i : 8;
        if(i > 5 && i < 8) {
            ++lx;
        }

        uint8_t ly = i < 8 ? 8 : 8 - (i - 7);
        if(i > 8) {
            --ly;
        }

        uint8_t sx = i < 7 ? 8                 : modules - 8 - 7 + i;
        uint8_t sy = i < 7 ? modules - i - 1 : 8;

        qr_reserve(out, lx, ly);
        qr_reserve(out, sx, sy);

        if(format[i]) {
            qr_set(out, lx, ly, false);
            qr_set(out, sx, sy, false);
        }
    }

    // if version >= 7,  we need more identifying data - the version information
    if(version >= 7) {
        for(uint8_t x = 0; x < 3; ++x) {
            for(uint8_t y = 0; y < 6; ++y) {
                qr_reserve(out, y, (modules-9)-x);
                qr_reserve(out, (modules-9)-x, y);
            }
        }

        for(size_t i = 0; i < 18; ++i) {
            uint8_t x = (18-i-1) / 3;
            uint8_t y = 2-((18-i-1) % 3);

            if(qr_versions[version].version_info & (1 << (17 - i))) {
                qr_set(out, x, modules-9-y, false);
                qr_set(out, modules-9-y, x, false);
            }
        }
    }
}

static inline bool qr_is_data(qr_data out, uint8_t version, uint8_t x, uint8_t y) {
    uint8_t modules = qr_modules(version);

    if(x < 8 && y < 8) return false;
    if(x >= modules - 8 && y < 8) return false;
    if(x < 8 && y >= modules - 8) return false;
    if(qr_reserved(out, x, y)) return false;
    return true;
}

static bool qr_mask_pattern0(uint8_t x, uint8_t y) {
    return ((x+y) % 2) == 0;
}

static bool qr_mask_pattern1(uint8_t x, uint8_t y) {
    return (y % 2) == 0;
}

static bool qr_mask_pattern2(uint8_t x, uint8_t y) {
    return (x % 3) == 0;
}

static bool qr_mask_pattern3(uint8_t x, uint8_t y) {
    return ((x+y) % 3) == 0;
}

static bool qr_mask_pattern4(uint8_t x, uint8_t y) {
    return (((y / 2) + (x / 3)) % 2) == 0;
}

static bool qr_mask_pattern5(uint8_t x, uint8_t y) {
    return (((y * x) % 2) + ((y * x) % 3)) == 0;
}

static bool qr_mask_pattern6(uint8_t x, uint8_t y) {
    return ((((y * x) % 2) + ((y * x) % 3)) % 2) == 0;
}

static bool qr_mask_pattern7(uint8_t x, uint8_t y) {
    return ((((y + x) % 2) + ((y * x) % 3)) % 2) == 0;
}

static void qr_mask(qr_data out, uint8_t version, uint8_t pattern) {
    if(pattern > 7) {
        loge("qr", "QR pattern %u not implemented", (unsigned)pattern);
    }

    uint8_t modules = qr_modules(version);

    bool(*patterns[])(uint8_t x, uint8_t y) = {
        qr_mask_pattern0,
        qr_mask_pattern1,
        qr_mask_pattern2,
        qr_mask_pattern3,
        qr_mask_pattern4,
        qr_mask_pattern5,
        qr_mask_pattern6,
        qr_mask_pattern7,
    };

    for(uint8_t y = 0; y < modules; ++y) {
        for(uint8_t x = 0; x < modules; ++x) {
            if(qr_is_data(out, modules, x, y)) {
                bool flip = patterns[pattern](x, y);

                if(flip) {
                    qr_flip(out, x, y);
                }
            }
        }
    }

    return;
}

static void qr_fixed_patterns(qr_data out, uint8_t version) {
    uint8_t modules = qr_modules(version);

    qr_finder(out, 0, 0);
    qr_finder(out, modules - 7, 0);
    qr_finder(out, 0, modules - 7);

    // timing
    for(uint8_t i = 8; i < modules - 8; i++) {
        qr_reserve(out, i, 6);
        qr_reserve(out, 6, i);

        if(!(i % 2)) {
            qr_set(out, i, 6, false);
            qr_set(out, 6, i, false);
        }
    }

    uint8_t* alignment = qr_versions[version].alignment_locations;
    size_t alignment_patterns;
    for(alignment_patterns = 0; alignment[alignment_patterns] != 0; ++alignment_patterns);

    for(size_t x = 0; x < alignment_patterns; ++x) {
        for(size_t y = 0; y < alignment_patterns; ++y) {
            if(
                (x == 0 && y == 0) ||
                (x == alignment_patterns-1 && y == 0) ||
                (y == alignment_patterns-1 && x == 0)
            ) {
                continue;
            }

            qr_alignment(out, alignment[x], alignment[y]);
        }
    }

    // dark module
    qr_reserve(out, 8, modules - 8);
    qr_set(out, 8, modules - 8, false);
}

static uint8_t qr_determine_mask(qr_data out, uint8_t version) {
    return 0;
}

static inline uint8_t qr_right_column(uint8_t x) {
    return (x % 2) == (x < 6 ? 1 : 0) ? x : x + 1;
}

static inline bool qr_upwards(uint8_t version, uint8_t x) {
    uint8_t modules = qr_modules(version);
    uint8_t cols_total = modules / 2;
    bool ret = (cols_total % 2) == ((qr_right_column(x) / 2) % 2);

    if(x < 6) {
        ret = !ret;
    }

    return ret;
}

static inline void qr_next_module(qr_data out, uint8_t version, uint8_t* x, uint8_t* y) {
    uint8_t modules      = qr_modules(version);
    uint8_t right_column = qr_right_column(*x);
    bool upwards         = qr_upwards(version, *x);

    bool is_right_col = *x == right_column;

    if(is_right_col) {
        --*x;
    } else {
        ++*x;
        if(upwards) {
            if(*y) {
                --*y;
            } else {
                *x -= 2;
            }
        }
        else {
            if(*y < modules - 1) {
                ++*y;
            } else {
                *x -= 2;
            }
        }
    }

    if(*x == 6) {
        --*x;
    }

    if(qr_reserved(out, *x, *y)) {
        qr_next_module(out, version, x, y);
    }
}

static inline void qr_store_bit(qr_data out, uint8_t version, uint8_t *x, uint8_t* y, bool bit) {
    if(bit) {
        qr_set(out, *x, *y, true);
    }

    qr_next_module(out, version, x, y);
}

static void qr_store_data(qr_data out, uint8_t version, const bitmap_t data) {
    uint8_t modules = qr_modules(version);
    uint8_t y       = modules - 1;
    uint8_t x       = modules - 1;

    for(size_t i = 0; i < qr_all_codewords(version) * 8; ++i) {
        qr_store_bit(out, version, &x, &y, bitmap_get(data, i));
    }
}

static size_t qr_interleaved_data_index(uint8_t version, size_t codeword_index) {
    size_t total_blocks     = qr_versions[version].block_sizes[0]
                            + qr_versions[version].block_sizes[2];

    size_t group1_codewords = qr_versions[version].block_sizes[0]
                            * qr_versions[version].block_sizes[1];

    size_t block     = codeword_index / qr_versions[version].block_sizes[1];
    size_t block_idx = codeword_index % qr_versions[version].block_sizes[1];

    size_t lblock = block;

    if(codeword_index > group1_codewords) {
        lblock    = codeword_index / qr_versions[version].block_sizes[3];
        block     = qr_versions[version].block_sizes[0];
        block_idx = codeword_index % qr_versions[version].block_sizes[3];
    }

    if(block_idx < qr_versions[version].block_sizes[1]) {
        return (total_blocks * block_idx) + block;
    } else {
        size_t offset = group1_codewords;
        return offset + lblock;
    }
}

static void qr_encode_data(qr_data out, uint8_t version, const uint8_t* data, size_t data_length) {
    uint8_t data_codewords[qr_data_codewords(version)];
    memset(data_codewords, 0, sizeof(data_codewords));

    size_t bit = 0;

    uint8_t mode_indicator = 4; // binary data
    for(size_t i = 0; i < 4; ++i && ++bit) {
        if(mode_indicator & (1 << (3 - i))) {
            bitmap_set((bitmap_t)data_codewords, bit);
        }
    }

    size_t datalen_bits = version <= 9 ? 8 : 16;
    for(size_t i = 0; i < datalen_bits; ++i && ++bit) {
        if(data_length & (1 << (datalen_bits - i - 1))) {
            bitmap_set((bitmap_t)data_codewords, bit);
        }
    }

    for(size_t i = 0; i < data_length; ++i) {
        for(size_t j = 0; j < 8; ++j && ++bit) {
            if(data[i] & (1 << (7 - j))) {
                bitmap_set((bitmap_t)data_codewords, bit);
            }
        }
    }

    // terminator bits to pad to full bytes - this is a shortcut we can take
    // as we always add full bytes, so we are always off by 4 bits from the
    // mode indicator.
    bit += 4;

    for(size_t i = 0; bit < sizeof(data_codewords) * 8; ++i) {
        uint8_t byte = i % 2 ? 17 : 236;

        for(size_t j = 0; j < 8; ++j && ++bit) {
            if(byte & (1 << (7 - j))) {
                bitmap_set((bitmap_t)data_codewords, bit);
            }
        }
    }

    uint8_t final_codewords[qr_all_codewords(version)];
    memset(final_codewords, 0, sizeof(final_codewords));

    size_t num_ec_codewords = qr_versions[version].ec_codewords_per_block;
    size_t total_blocks = qr_versions[version].block_sizes[0] + qr_versions[version].block_sizes[2];
    size_t total_block = 0, si = 0;
    uint8_t* blocks = qr_versions[version].block_sizes;

    while(*blocks) {
        uint8_t block_count = blocks[0];
        uint8_t block_size  = blocks[1];
        for(uint8_t block = 0; block < block_count; ++block && ++total_block) {
            uint8_t ec_codewords[num_ec_codewords];
            qr_block_ec_generate(data_codewords + si, block_size, ec_codewords, num_ec_codewords);

            for(uint8_t i = 0; i < block_size; ++i) {
                size_t di = qr_interleaved_data_index(version, si);

                final_codewords[di] = data_codewords[si++];
            }

            for(uint8_t i = 0; i < sizeof(ec_codewords); ++i) {
                size_t di = sizeof(data_codewords)
                          + (total_blocks * i)
                          + total_block;
                final_codewords[di] = ec_codewords[i];
            }
        }

        blocks += 2;
    }

    qr_store_data(out, version, final_codewords);
}

static uint8_t qr_encode(qr_data out, const uint8_t* data, size_t data_length) {
    memset(out, 0, sizeof(qr_data));

    uint8_t version = qr_version(data_length);

    qr_fixed_patterns(out, version);
    uint8_t masking_pattern = qr_determine_mask(out, version);
    qr_format_and_version(out, version, masking_pattern);

    qr_encode_data(out, version, data, data_length);

    qr_mask(out, version, masking_pattern);

    return qr_modules(version);
}

uint8_t qr_log(qr_data out) {
    const char* data = "LF OS rocks!\nwith quite some more random text and data to get a bigger QR code that might just decode without error correction coding?";
    return qr_encode(out, (const uint8_t*)data, strlen(data));
}
