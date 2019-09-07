#ifndef _ELF_H_INCLUDED
#define _ELF_H_INCLUDED

#include "stdint.h"
#include "vm.h"

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

typedef struct {
    uint32_t type;
    uint32_t flags;

    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t fileLength;
    uint64_t memLength;

    uint64_t align;
}__attribute__((packed)) elf_program_header_t;

typedef struct {
    uint32_t name;
    uint32_t type;
    uint64_t flags;
    ptr_t    addr;
    uint64_t offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t addralign;
    uint64_t entrySize;
}__attribute__((packed)) elf_section_header_t;

typedef struct {
    uint32_t name;
    uint8_t  info;
    uint8_t  other;
    uint16_t sectionHeader;
    uint64_t addr;
    uint64_t size;
}__attribute__((packed)) elf_symbol_t;

ptr_t load_elf(ptr_t start, vm_table_t* context);

elf_section_header_t* elf_section_by_name(const char* name, const void* elf);

#endif
