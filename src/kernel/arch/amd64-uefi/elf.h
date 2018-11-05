#ifndef _ELF_H_INCLUDED
#define _ELF_H_INCLUDED

#include <stdint.h>

#define ELF_MAGIC 0x464c457f

typedef struct {
    uint32_t ident_magic;
    uint8_t  ident_arch;
    uint8_t  ident_byteOrder;
    uint8_t  ident_version;

    uint8_t  ident_abi;
    uint8_t  ident_abi_version;

    uint8_t  _ident_padding[7];

    uint16_t type;
    uint16_t machine;

    uint32_t version;

    ptr_t    entrypoint;
    ptr_t    programHeaderOffset;
    ptr_t    sectionHeaderOffset;

    uint32_t flags;
    uint16_t headerSize;

    uint16_t programHeaderEntrySize;
    uint16_t programHeaderCount;

    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderCount;
    uint16_t sectionHeaderSectionNameIndex;
}__attribute__((packed)) elf_file_header_t;

void load_elf(ptr_t start);

#endif
