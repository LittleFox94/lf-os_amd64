#include "elf.h"
#include "fbconsole.h"
#include "string.h"

void load_elf(ptr_t start) {
    elf_file_header_t* header = (elf_file_header_t*)start;

    if(header->ident_magic != ELF_MAGIC) {
        fbconsole_write("[ELF] not an ELF file, invalid magic (%x != %x)!\n", header->ident_magic, ELF_MAGIC);
        return;
    }

    if(header->machine != 0x3E || header->version != 1) {
        fbconsole_write("[ELF] incompatible ELF file, invalid machine or version\n");
        return;
    }

    // check if file is an executable
    if(header->type != 0x02) {
        fbconsole_write("[ELF] file not executable (%u != %u)\n", header->type, 2);
        return;
    }

    int phStart = 0;
    for(int i = 0; i < header->programHeaderCount; ++i) {
        elf_program_header_t* programHeader = (elf_program_header_t*)(start + header->programHeaderOffset + phStart);

        if(programHeader->type != 1) {
            continue;
        }

        fbconsole_write("[ELF] len: 0x%x, offset: 0x%x, vaddr: 0x%x\n", programHeader->fileLength, programHeader->offset, programHeader->vaddr);

        memcpy((void*)programHeader->vaddr, (void*)(start + programHeader->offset), programHeader->fileLength);
        phStart += header->programHeaderEntrySize;
    }

    asm("mov $0x2000,%rsp");

    void (*entry)() = (void(*)())header->entrypoint;
    entry();

    fbconsole_write("[ELF] program executed\n");
}
