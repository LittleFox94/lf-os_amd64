#ifndef _IMAGE_READER_H_INCLUDED
#define _IMAGE_READER_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <elf.h>

typedef struct {
    uint64_t address;
    char     name[248];
} Symbol;

typedef struct {
    uint64_t entry;
    FILE*    image_file;

    Elf64_Ehdr  file_header;

    Elf64_Shdr* section_headers;

    Symbol* symbols;
    size_t  num_symbols;
} ImageReader;

int image_reader_init(const char* filepath, ImageReader* reader);
Symbol* image_reader_get_symbol(ImageReader* reader, uint64_t addr);

#endif
