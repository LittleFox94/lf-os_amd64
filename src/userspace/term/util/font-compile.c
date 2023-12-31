#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

struct bbox {
    int         width;
    int         height;
    int         lower_left_x;
    int         lower_left_y;
};

struct glyph {
    char       *name;
    uint32_t    encoding;
    uint64_t   *bitmap;
};

struct font {
    char           *name;
    size_t          num_glyphs;
    struct bbox     bbox;
    struct glyph   *glyphs;
};

void parse_bbox(struct bbox* bbox, char *line, char **toksave) {
    char *width  = strtok_r(0, " \n", toksave);
    char *height = strtok_r(0, " \n", toksave);
    char *ll_x   = strtok_r(0, " \n", toksave);
    char *ll_y   = strtok_r(0, " \n", toksave);

    bbox->width        = strtol(width, 0, 10);
    bbox->height       = strtol(height, 0, 10);
    bbox->lower_left_x = strtol(ll_x, 0, 10);
    bbox->lower_left_y = strtol(ll_y, 0, 10);
}

void dump_font_as_c_code(struct font *font, FILE *stream) {
    fprintf(stream, "#include \"font.h\"\n");

    fprintf(stream, "struct font terminal_font = (struct font){\n");
    fprintf(stream, "    .name = \"%s\",\n", font->name);
    fprintf(stream, "    .bbox = (struct bbox){%d, %d, %d, %d},\n", font->bbox.width, font->bbox.height, font->bbox.lower_left_x, font->bbox.lower_left_y);
    fprintf(stream, "    .num_glyphs = %zu,\n", font->num_glyphs);
    fprintf(stream, "    .glyphs = (struct glyph[]) {\n");

    for(size_t i = 0; i < font->num_glyphs; ++i) {
        struct glyph *glyph = &font->glyphs[i];
        fprintf(stream, "        (struct glyph) {\n");
        fprintf(stream, "            .name = \"%s\",\n", glyph->name);
        fprintf(stream, "            .encoding = %u,\n", glyph->encoding);
        fprintf(stream, "            .bitmap = (uint64_t[]) {\n");

        for(size_t j = 0; j < font->bbox.height; ++j) {
            fprintf(stream, "                %#lx,\n", glyph->bitmap[j]);
        }

        fprintf(stream, "            },\n");
        fprintf(stream, "        },\n");
    }

    fprintf(stream, "    }\n");
    fprintf(stream, "};\n");
}

int main(int argc, char* argv[]) {
    struct font  font;
    bzero(&font, sizeof(font));

    struct glyph *current_glyph;

    int lineNumber = 0;
    char line[256];

    while(fgets(line, sizeof(line), stdin)) {
        ++lineNumber;

        char *toksave = 0;
        char *command = strtok_r(line, " \n", &toksave);
        if(!command) continue;

        if(strcmp(command, "FONT") == 0) {
            const char *fontName = strtok_r(0, "\n", &toksave);
            font.name = strdup(fontName);
        } else if(strcmp(command, "CHARS") == 0) {
            const char *chars = strtok_r(0, " \n", &toksave);

            // let's do some good-weather programming and assume we always get
            // a valid number!
            font.num_glyphs = strtoul(chars, 0, 10);
            font.glyphs     = malloc(sizeof(font.glyphs[0]) * font.num_glyphs);
            current_glyph   = font.glyphs;
        } else if(strcmp(command, "FONTBOUNDINGBOX") == 0) {
            parse_bbox(&font.bbox, line, &toksave);
        } else if(strcmp(command, "BBOX") == 0) {
            struct bbox bbox;
            parse_bbox(&bbox, line, &toksave);

            if(bbox.height && bbox.height != font.bbox.height) {
                fprintf(stderr, "Glyph bounding box not matching font bound box is not supported (line number %u)\n", lineNumber);
                return -1;
            }
        } else if(strcmp(command, "STARTCHAR") == 0) {
            const char *glyphName = strtok_r(0, "\n", &toksave);
            current_glyph->name = strdup(glyphName);
        } else if(strcmp(command, "ENCODING") == 0) {
            const char *encoding = strtok_r(0, " \n", &toksave);

            // let's do some good-weather programming and assume we always get
            // a valid number!
            current_glyph->encoding = strtoul(encoding, 0, 10);
        } else if(strcmp(command, "BITMAP") == 0) {
            size_t bitmap_size = font.bbox.height;
            if(!bitmap_size) {
                fprintf(stderr, "Don't know the size of each glyphs bitmap, because font bounding box not set or 0 height, cannot continue\n");
                return -1;
            }

            current_glyph->bitmap = malloc(sizeof(current_glyph->bitmap[0]) * bitmap_size);

            for(size_t i = 0; i < bitmap_size; ++i) {
                ++lineNumber;

                char bitmapEntry[16];
                if(!fgets(bitmapEntry, sizeof(bitmapEntry), stdin)) {
                    fprintf(stderr, "Error reading bitmap entry from file (line number %u)\n", lineNumber);
                    return -1;
                }

                char* reject = 0;
                uint64_t entry = strtoull(bitmapEntry, &reject, 16);

                if(reject == bitmapEntry) {
                    fprintf(stderr, "Cannot parse bitmap entry in line %u as hex number: %s\n", lineNumber, bitmapEntry);
                    return -1;
                }

                current_glyph->bitmap[i] = entry;
            }
        } else if(strcmp(command, "ENDCHAR") == 0) {
            ++current_glyph;
        }
    }

    dump_font_as_c_code(&font, stdout);

    return 0;
}
