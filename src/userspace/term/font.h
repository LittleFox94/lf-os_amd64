#include <stdint.h>
#include <stdlib.h>

struct glyph {
    char     *name;
    uint32_t  encoding;
    uint64_t *bitmap;
};

struct bbox {
    int width, height, lower_left_x, lower_left_y;
};

struct font {
    char           *name;
    struct bbox     bbox;
    size_t          num_glyphs;
    struct glyph   *glyphs;
};
