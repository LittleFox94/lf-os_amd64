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

        if(programHeader->type != 1) {
            continue;
        }

        for(size_t j = 0; j < programHeader->memLength; j += 0x1000) {
            ptr_t physical = (ptr_t)mm_alloc_pages(1);
            memset((void*)(physical + ALLOCATOR_REGION_DIRECT_MAPPING.start), 0, 0x1000);
            vm_context_map(context, (ptr_t)programHeader->vaddr + j, physical);

            if(j < programHeader->fileLength) {
                size_t offset = 0;
                size_t toCopy = programHeader->fileLength - j;
                if(toCopy > 0x1000) toCopy = 0x1000;

                if((programHeader->vaddr + j) & 0xFFF) {
                    if(toCopy > ((programHeader->vaddr + j) & 0xFFF))
                        toCopy -= (programHeader->vaddr + j) & 0xFFF;

                    offset  = (programHeader->vaddr + j) & 0xFFF;
                }

                memcpy((void*)(offset + physical + ALLOCATOR_REGION_DIRECT_MAPPING.start), (void*)(start + programHeader->offset + j), toCopy);

                if((programHeader->vaddr + j) & 0xFFF) {
                    j -= (programHeader->vaddr + j) & 0xFFF;
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

    elf_section_header_t* shSectionNames = (elf_section_header_t*)(elf + eh->sectionHeaderOffset + (eh->sectionHeaderEntrySize * eh->sectionHeaderSectionNameIndex));
    char* sectionNames                   = (char*)(elf + shSectionNames->offset);

    for(size_t i = 0; i < eh->sectionHeaderCount; ++i) {
        elf_section_header_t* sh = (elf_section_header_t*)(elf + eh->sectionHeaderOffset + (eh->sectionHeaderEntrySize * i));

        if(strcmp(sectionNames + sh->name, name) == 0) {
            return sh;
        }
    }

    return 0;
}
