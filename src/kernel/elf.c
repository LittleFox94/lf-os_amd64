#include "elf.h"
#include "fbconsole.h"
#include "string.h"
#include "mm.h"
#include "vm.h"

ptr_t load_elf(ptr_t start, vm_table_t* context) {
    elf_file_header_t* header = (elf_file_header_t*)start;

    if(header->ident_magic != ELF_MAGIC) {
        fbconsole_write("[ELF] not an ELF file, invalid magic (%x != %x)!\n", header->ident_magic, ELF_MAGIC);
        return 0;
    }

    if(header->machine != 0x3E || header->version != 1) {
        fbconsole_write("[ELF] incompatible ELF file, invalid machine or version\n");
        return 0;
    }

    // check if file is an executable
    if(header->type != 0x02) {
        fbconsole_write("[ELF] file not executable (%u != %u)\n", header->type, 2);
        return 0;
    }

    int phStart = 0;
    for(int i = 0; i < header->programHeaderCount; ++i) {
        elf_program_header_t* programHeader = (elf_program_header_t*)(start + header->programHeaderOffset + phStart);

        if(programHeader->type != 1) {
            continue;
        }

        for(size_t i = 0; i < programHeader->fileLength; i += 4096) {
            vm_context_map(context, (ptr_t)programHeader->vaddr + i, vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, start + programHeader->offset + i));
        }

        phStart += header->programHeaderEntrySize;
    }

    return header->entrypoint;
}

elf_section_header_t* elf_section_by_name(const char* name, const void* elf) {
    elf_file_header_t* eh = (elf_file_header_t*)elf;

    elf_section_header_t* shSectionNames = (elf_section_header_t*)(elf + eh->sectionHeaderOffset + (eh->sectionHeaderEntrySize * eh->sectionHeaderSectionNameIndex));
    char* sectionNames                   = (char*)(elf + shSectionNames->offset);

    for(size_t i = 0; i < eh->sectionHeaderCount; ++i) {
        elf_section_header_t* sh = (elf_section_header_t*)(elf + eh->sectionHeaderOffset + (eh->sectionHeaderEntrySize * i));

        if(__builtin_strcmp(sectionNames + sh->name, name) == 0) {
            return sh;
        }
    }

    return 0;
}
