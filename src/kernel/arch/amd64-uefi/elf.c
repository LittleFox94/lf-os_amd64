#include "elf.h"
#include "fbconsole.h"

void load_elf(ptr_t start) {
    elf_file_header_t* header = (elf_file_header_t*)start;

    if(header->ident_magic != ELF_MAGIC) {
        fbconsole_write("[ELF] not an ELF file, invalid magic (%x != %x)!\n", header->ident_magic, ELF_MAGIC);
    }

    // check if file is an executable
    if(header->type != 0x02) {
        fbconsole_write("[ELF] file not executable (%u != %u)\n", header->type, 2);
    }
}
