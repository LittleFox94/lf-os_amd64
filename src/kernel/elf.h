#ifndef _ELF_H_INCLUDED
#define _ELF_H_INCLUDED

#include <stdint.h>
#include <vm.h>
#include <allocator.h>
#include <memory/context.h>

#include <memory>

#define ELF_MAGIC 0x464c457f

//! Header of ELF images
struct elf_file_header {
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

    uint64_t entrypoint;
    uint64_t programHeaderOffset;
    uint64_t sectionHeaderOffset;

    uint32_t flags;
    uint16_t headerSize;

    uint16_t programHeaderEntrySize;
    uint16_t programHeaderCount;

    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderCount;
    uint16_t sectionHeaderSectionNameIndex;
}__attribute__((packed));
typedef struct elf_file_header elf_file_header_t;

//! Program headers in ELF images, required for preparing a program for execution
struct elf_program_header {
    uint32_t type;
    uint32_t flags;

    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t fileLength;
    uint64_t memLength;

    uint64_t align;
}__attribute__((packed));
typedef struct elf_program_header elf_program_header_t;

//! Section headers in ELF images, required to read additional information from file
struct elf_section_header {
    uint32_t name;
    uint32_t type;
    uint64_t flags;
    uint64_t addr;
    uint64_t offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t addralign;
    uint64_t entrySize;
}__attribute__((packed));
typedef struct elf_section_header elf_section_header_t;

//! Symbols in ELF images, there may be many
struct elf_symbol {
    uint32_t name;
    uint8_t  info;
    uint8_t  other;
    uint16_t sectionHeader;
    uint64_t addr;
    uint64_t size;
}__attribute__((packed));
typedef struct elf_symbol elf_symbol_t;

/**
 * Parse ELF file, map program segments to context and return proper location for stack.
 *
 * \param[in]  elf          Memory chunk where the ELF file is loaded.
 * \param[in]  context      VM context for mapping of program segments.
 * \returns                 Entrypoint of the image
 */
uint64_t load_elf(uint8_t* elf, std::shared_ptr<MemoryContext> context);

/**
 * Return section header of ELF file by name
 *
 * \param name Name of the section to return
 * \param elf  Loaded ELF image in memory
 * \returns    Pointer to elf_section_header_t if found, NULL otherwise
 */
elf_section_header_t* elf_section_by_name(const char* name, const void* elf);

/**
 * Load symbols of the provided ELF file.
 *
 * \param elf   ELF file already loaded into memory
 * \param alloc Allocator to use for dynamically allocating memory
 * \returns     Pointer to a opaque data structure containing the symbols,
 *              use elf_symbolize to retrieve the symbol for a given address
 */
void* elf_load_symbols(uint64_t elf, allocator_t* alloc);

/**
 * Get the symbol name (and offset) for a given address from the data loaded with
 * elf_load_symbols.
 *
 * \param symbols       Opaque data structure returned by elf_load_symbols
 * \param addr          Address to symbolize
 * \param symbol_length Input for size of the buffer where to write symbol, output actual
 *                      size of either the data written into the buffer or the size needed
 *                      for the buffer
 * \param symbol        Output buffer where to write the symbol
 * \returns             true if a symbol was found and the output buffer had enough space,
 *                      false if no symbol was found or output buffer was too small. Output
 *                      buffer to small can be recognized by symbol_length being set to a
 *                      non-zero value.
 */
bool elf_symbolize(void* symbols, uint64_t addr, size_t* symbol_length, char* symbol);

#endif
