#include "elf.h"
#include "log.h"
#include "string.h"
#include "mm.h"
#include "vm.h"

ptr_t load_elf(ptr_t start, struct vm_table* context, ptr_t* data_start, ptr_t* data_end) {
    elf_file_header_t* header = (elf_file_header_t*)start;

    if(header->ident_magic != ELF_MAGIC) {
        logf("elf", "not an ELF file, invalid magic (%x != %x)!", header->ident_magic, ELF_MAGIC);
        return 0;
    }

    if(header->machine != 0x3E || header->version != 1) {
        logf("elf", "incompatible ELF file, invalid machine or version");
        return 0;
    }

    // check if file is an executable
    if(header->type != 0x02) {
        logf("elf", "file not executable (%u != %u)", header->type, 2);
        return 0;
    }

    for(int i = 0; i < header->programHeaderCount; ++i) {
        elf_program_header_t* programHeader = (elf_program_header_t*)(start + header->programHeaderOffset + (i * header->programHeaderEntrySize));

        if(programHeader->type != 1 && programHeader->type != 7) {
            continue;
        }

        for(size_t j = 0; j < programHeader->memLength; j += 0x1000) {
            ptr_t physical = (ptr_t)mm_alloc_pages(1);
            memset((void*)(physical + ALLOCATOR_REGION_DIRECT_MAPPING.start), 0, 0x1000);
            vm_context_map(context, (ptr_t)programHeader->vaddr + j, physical, 0);

            if(j < programHeader->fileLength) {
                size_t offset = (programHeader->vaddr + j) & 0xFFF;
                size_t toCopy = programHeader->fileLength - j;

                if(toCopy > (0x1000 - offset)) {
                    toCopy = 0x1000 - offset;
                }

                memcpy((void*)(offset + physical + ALLOCATOR_REGION_DIRECT_MAPPING.start), (void*)(start + programHeader->offset + j), toCopy);

                if(offset) {
                    j -= offset;
                }
            }

        }

        ptr_t end = programHeader->vaddr + programHeader->memLength + 1;
        if(*data_end <= end) {
            *data_end = end;
        }

        if(programHeader->vaddr < *data_start) {
            *data_start = programHeader->vaddr;
        }
    }

    *data_end += 4096;
    *data_end &= ~0xFFF;

    return header->entrypoint;
}

elf_section_header_t* elf_section_by_name(const char* name, const void* elf) {
    elf_file_header_t* eh = (elf_file_header_t*)elf;

    // "Oh deer, who wrote this code" -- littlefox 2024-02-10 
    elf_section_header_t* shSectionNames = (elf_section_header_t*)((char*)elf + eh->sectionHeaderOffset + (eh->sectionHeaderEntrySize * eh->sectionHeaderSectionNameIndex));
    char* sectionNames                   = (char*)elf + shSectionNames->offset;

    for(size_t i = 0; i < eh->sectionHeaderCount; ++i) {
        elf_section_header_t* sh = (elf_section_header_t*)((char*)elf + eh->sectionHeaderOffset + (eh->sectionHeaderEntrySize * i));

        if(strcmp(sectionNames + sh->name, name) == 0) {
            return sh;
        }
    }

    return 0;
}

struct Symbol {
    uint64_t address;
    uint32_t name;
};

struct SymbolData {
    allocator_t*  alloc;
    uint64_t      numSymbols;
    char*         symbolNames;
    struct Symbol symbols[0];
};

void* elf_load_symbols(ptr_t elf, allocator_t* alloc) {
    elf_section_header_t* symtab = elf_section_by_name(".symtab", (void*)elf);
    elf_section_header_t* strtab = elf_section_by_name(".strtab", (void*)elf);

    if(symtab == 0 && strtab == 0) {
        logw("elf", "Unable to load symbols as either symtab (0x%x) or strtab (0x%x) are missing", symtab, strtab);
        return 0;
    }

    uint64_t numSymbols = symtab->size / symtab->entrySize;

    size_t dataSize = sizeof(struct SymbolData) + ((sizeof(struct Symbol) * numSymbols)) + strtab->size;
    void* data      = alloc->alloc(alloc, dataSize);

    struct SymbolData* header = (struct SymbolData*)data;
    header->alloc       = alloc;
    header->numSymbols  = numSymbols;
    header->symbolNames = (char*)data + sizeof(struct SymbolData) + (sizeof(struct Symbol) * numSymbols);

    memcpy(header->symbolNames, (char*)elf + strtab->offset, strtab->size);

    for(size_t i = 0; i < numSymbols; ++i) {
        elf_symbol_t* elfSymbol = (elf_symbol_t*)(elf + symtab->offset + (i * symtab->entrySize));
        struct Symbol* symbol   = (struct Symbol*)((char*)data + sizeof(struct SymbolData) + (sizeof(struct Symbol) * i));

        symbol->address = elfSymbol->addr;
        symbol->name    = elfSymbol->name;
    }

    return (void*)data;
}

bool elf_symbolize(void* symbol_data, ptr_t addr, size_t* symbol_size, char* symbol) {
    if(!symbol_data) {
        *symbol_size = 0;
        return false;
    }

    struct SymbolData* symbols = (struct SymbolData*)symbol_data;

    int64_t best_difference = ~(1ULL<<63);
    struct Symbol* best     = 0;

    for(size_t i = 0; i < symbols->numSymbols; ++i) {
        struct Symbol* current = &symbols->symbols[i];
        int64_t diff           = addr - current->address;

        if(diff > 0 && diff < best_difference) {
            best            = current;
            best_difference = diff;
        }
    }

    if(!best) {
        *symbol_size = 0;
        return false;
    }

    char* best_name = symbols->symbolNames + best->name;
    size_t max_size = *symbol_size;

    if(best_difference) {
        *symbol_size = ksnprintf(symbol, *symbol_size, "%s(+0x%x)", best_name, best_difference);
    } else {
        *symbol_size = strlen(best_name) + 1;

        strncpy(symbol, best_name, max_size);
    }

    return *symbol_size <= max_size;
}
