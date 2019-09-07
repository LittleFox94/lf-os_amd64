#ifndef _BLUESCREEN_H_INCLUDED
#define _BLUESCREEN_H_INCLUDED

#include "cpu.h"
#include "elf.h"

__attribute__((noreturn)) void panic();
__attribute__((noreturn)) void panic_message(const char* message);

__attribute__((noreturn)) void panic_cpu(const cpu_state* cpu);

void bluescreen_load_symbols(void* elf, elf_section_header_t* symtab, elf_section_header_t* strtab);

#endif
