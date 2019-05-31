#ifndef _ELF_H_INCLUDED
#define _ELF_H_INCLUDED

/**
 * Minimal elf file parser and runtime
 */

#include <stdint.h>

static const uint8_t* ELF_EXPECED_IDENT = {0x7F, 'E', 'L', 'F', 2, 1, 1, 0xFF, 0, 0, 0, 0, 0, 0, 0, 16};

typedef struct {
    uint8_t     e_ident[16];
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    ptr_t       e_entry;
    uint64_t    e_phoff;
    uint64_t    e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
} elf_header_t;

#endif
