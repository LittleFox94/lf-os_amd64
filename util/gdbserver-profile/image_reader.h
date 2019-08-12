#ifndef _IMAGE_READER_H_INCLUDED
#define _IMAGE_READER_H_INCLUDED

#include <stdint.h>

typedef struct {
    uint64_t entry;
} ImageReader;

int image_reader_init(const char* filepath, ImageReader* reader);

#endif
