#include <elf.h>
#include <stdio.h>

#include "image_reader.h"

int image_reader_init(const char* image_path, ImageReader* reader) {
    FILE* image = fopen(image_path, "rb");

    if(image == NULL) {
        return -1;
    }

    Elf64_Ehdr file_header;
    fread(&file_header, sizeof(file_header), 1, image);
    fclose(image);

    reader->entry = file_header.e_entry;

    return 0;
}
