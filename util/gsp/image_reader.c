#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "image_reader.h"

int image_reader_compare_symbols(const void* a, const void* b) {
    const Symbol* sa = (Symbol*)a;
    const Symbol* sb = (Symbol*)b;

    if(sa->address == sb->address)
        return 0;

    if(sa->address < sb->address)
        return -1;

    return 1;
}

void image_reader_read_symbols(ImageReader* reader, Elf64_Shdr* symbol_section, Elf64_Shdr* string_section) {
    char* symbol_names = malloc(string_section->sh_size);
    fseeko(reader->image_file, string_section->sh_offset, SEEK_SET);
    fread(symbol_names, string_section->sh_size, 1, reader->image_file);

    reader->num_symbols = symbol_section->sh_size / symbol_section->sh_entsize;

    Elf64_Sym* symbols = malloc(symbol_section->sh_entsize * reader->num_symbols);
    fseeko(reader->image_file, symbol_section->sh_offset, SEEK_SET);
    fread(symbols, symbol_section->sh_entsize, reader->num_symbols, reader->image_file);

    reader->symbols = malloc(reader->num_symbols * sizeof(Symbol));
    memset(reader->symbols, 0, reader->num_symbols * sizeof(Symbol));

    for(size_t i = 0; i < reader->num_symbols; ++i) {
        reader->symbols[i].address = symbols[i].st_value;
        strncpy(reader->symbols[i].name, symbol_names + symbols[i].st_name, sizeof(reader->symbols[i].name));
    }

    free(symbols);
    free(symbol_names);

    qsort(reader->symbols, reader->num_symbols, sizeof(Symbol), image_reader_compare_symbols);
}

int image_reader_init(const char* image_path, ImageReader* reader) {
    memset(reader, 0, sizeof(ImageReader));

    if((reader->image_file = fopen(image_path, "rb")) == NULL) {
        return -1;
    }

    fread(&reader->file_header, sizeof(reader->file_header), 1, reader->image_file);

    reader->entry = reader->file_header.e_entry;

    reader->section_headers = malloc(reader->file_header.e_shentsize * reader->file_header.e_shnum);
    off_t offset = reader->file_header.e_shoff;
    fseeko(reader->image_file, offset, SEEK_SET);
    fread(reader->section_headers, reader->file_header.e_shentsize, reader->file_header.e_shnum, reader->image_file);

    char* section_names = NULL;
    size_t section_names_len = 0;
    if(reader->file_header.e_shstrndx != SHN_UNDEF) {
        Elf64_Shdr* section = (Elf64_Shdr*)((uint64_t)reader->section_headers + (reader->file_header.e_shstrndx * reader->file_header.e_shentsize));

        section_names_len = section->sh_size;
        section_names = malloc(section_names_len);
        fseeko(reader->image_file, section->sh_offset, SEEK_SET);
        fread(section_names, 1, section_names_len, reader->image_file);
    }

    uint16_t sh_index_symtab = SHN_UNDEF;;
    uint16_t sh_index_strtab = SHN_UNDEF;;

    for(size_t i = 0; i < reader->file_header.e_shnum; ++i) {
        Elf64_Shdr* section = (Elf64_Shdr*)((uint64_t)reader->section_headers + (i * reader->file_header.e_shentsize));

        if(strcmp(section_names + section->sh_name, ".symtab") == 0) {
            sh_index_symtab = i;
        }
        else if(strcmp(section_names + section->sh_name, ".strtab") == 0) {
            sh_index_strtab = i;
        }
    }

    Elf64_Shdr *string_section = (Elf64_Shdr*)((uint64_t)reader->section_headers + (sh_index_strtab * reader->file_header.e_shentsize));
    Elf64_Shdr *symbol_section = (Elf64_Shdr*)((uint64_t)reader->section_headers + (sh_index_symtab * reader->file_header.e_shentsize));

    image_reader_read_symbols(reader, symbol_section, string_section);

    return 0;
}

Symbol* image_reader_get_symbol(ImageReader* reader, uint64_t addr) {
    if(reader->num_symbols == 0) {
        return NULL;
    }

    for(size_t i = 0; i < reader->num_symbols - 1; ++i) {
        if(reader->symbols[i + 1].address > addr) {
            return &reader->symbols[i];
        }
    }

    return &reader->symbols[reader->num_symbols - 1];
}
