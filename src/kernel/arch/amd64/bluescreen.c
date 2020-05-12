#include "bluescreen.h"
#include "config.h"
#include "string.h"
#include "fbconsole.h"

extern const char* LAST_INIT_STEP;

struct StackFrame {
    struct StackFrame* prev;
    ptr_t              rip;
};

struct Symbol {
    uint64_t address;
    uint32_t name;
};

struct SymbolData {
    uint64_t      numSymbols;
    char*         symbolNames;
    struct Symbol symbols[0];
};

struct SymbolData* bluescreen_symbols = 0;

void _panic_message(const char* message, uint64_t rbp, bool rbp_given) {
    //fbconsole_clear(0, 0, 127);
    fbconsole_write("\n\e[38;5;9m\e[48;5;0m");
    fbconsole_write("An error occured and LF OS has to be halted.\n"
                    "Below you can find more information:\n\n");

    fbconsole_write("LF OS build:    %s\n",   BUILD_ID);
    fbconsole_write("Last init step: %s\n",   LAST_INIT_STEP);
    fbconsole_write("Error message:  %s\n\n", message);

    struct StackFrame* frame;

    if(!rbp_given) {
        asm("mov %%rbp,%0":"=r"(frame));
    }
    else {
        frame = (struct StackFrame*)rbp;
    }

    if(frame) {
        fbconsole_write("\nStack trace that led to this error:\n");

        int i = 20;
        while(frame && frame->rip > 0xFFFF800000000000 && --i) {
            fbconsole_write("\e[38;5;7m  0x%016x", frame->rip);

            if(bluescreen_symbols != 0) {
                int64_t difference = 0x7FFFFFFFFFFFFFFF;
                struct Symbol* best = 0;

                for(size_t i = 0; i < bluescreen_symbols->numSymbols; ++i) {
                    struct Symbol* current = (struct Symbol*)((ptr_t)bluescreen_symbols->symbols + (sizeof(struct Symbol) * i));

                    if(frame->rip - current->address > 0) {
                        if(difference > frame->rip - current->address) {
                            best = current;
                        }
                    }
                }

                if(best) {
                    fbconsole_write("\e[38;5;15m %s(+0x%x)", bluescreen_symbols->symbolNames + best->name, frame->rip - best->address);
                }
            }

            fbconsole_write("\n");

            if(frame->prev != frame) {
                frame = frame->prev;
            }
            else {
                frame = 0;
            }
        }
    }

    fbconsole_write("\n");
}

void panic() {
    panic_message("Unknown error");
}

void panic_message(const char* message) {
    _panic_message(message, 0, false);
    while(1) {
        asm("hlt");
    }
}

void panic_cpu(const cpu_state* cpu) {
    const char* exceptions[] = {
        "Division by zero",
        "Debug",
        "NMI",
        "Breakpoint",
        "Overflow",
        "Bound range exceeded",
        "Invalid Opcode",
        "Device not available",
        "Double fault",
        "Coprocessor segment overrun",
        "Invalid TSS",
        "Segment not present",
        "Stack-segment fault",
        "General protection fault",
        "Page fault",
        "reserved",
        "x87 floating point exception",
        "Alignment check",
        "Machine check",
        "SIMD floating point exception",
        "Virtualization exception",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "Security exception",
        "reserved"
    };

    char cr2_msg[256] = { 0 };

    if(cpu->interrupt == 0x0E) {
        uint64_t cr2;
        asm("mov %%cr2, %0":"=r"(cr2));

        if(bluescreen_symbols != 0 && cr2 != 0) {
            int64_t difference = 0x7FFFFFFFFFFFFFFF;
            struct Symbol* best = 0;

            for(size_t i = 0; i < bluescreen_symbols->numSymbols; ++i) {
                struct Symbol* current = (struct Symbol*)((ptr_t)bluescreen_symbols->symbols + (sizeof(struct Symbol) * i));

                if(cr2 - current->address > 0) {
                    if(difference > cr2 - current->address) {
                        best = current;
                    }
                }
            }

            ksnprintf(cr2_msg, 256, " @ 0x%016x, %s(+0x%x)", cr2, bluescreen_symbols->symbolNames + best->name, cr2 - best->address);
        }
        else {
            ksnprintf(cr2_msg, 256, " @ 0x%016x", cr2);
        }

    }

    char message[256];
    ksnprintf(message, 256, "Interrupt: 0x%02x (%s), error: 0x%04x%s", cpu->interrupt, exceptions[cpu->interrupt], cpu->error_code, cr2_msg);
    _panic_message(message, cpu->rbp, true);

    if(bluescreen_symbols != 0) {
        int64_t difference = 0x7FFFFFFFFFFFFFFF;
        struct Symbol* best = 0;

        for(size_t i = 0; i < bluescreen_symbols->numSymbols; ++i) {
            struct Symbol* current = (struct Symbol*)((ptr_t)bluescreen_symbols->symbols + (sizeof(struct Symbol) * i));

            if(cpu->rip - current->address > 0) {
                if(difference > cpu->rip - current->address) {
                    best = current;
                }
            }
        }

        if(best) {
            fbconsole_write("\e[38;7;7mRIP: \e[38;5;15m %s(+0x%x)\n\n", bluescreen_symbols->symbolNames + best->name, cpu->rip - best->address);
        }
    }

    DUMP_CPU(cpu);

    while(1) {
        asm("hlt");
    }
}

void bluescreen_load_symbols(void* elf, elf_section_header_t* symtab, elf_section_header_t* strtab) {
    uint64_t numSymbols = symtab->size / symtab->entrySize;

    size_t dataSize = sizeof(struct SymbolData) + ((sizeof(struct Symbol) * numSymbols)) + strtab->size + 4095;
    ptr_t  data     = vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, dataSize / 4096);

    struct SymbolData* header = (struct SymbolData*)data;
    header->numSymbols  = numSymbols;
    header->symbolNames = (char*)(data + sizeof(struct SymbolData) + (sizeof(struct Symbol) * numSymbols));

    memcpy(header->symbolNames, elf + strtab->offset, strtab->size);

    for(size_t i = 0; i < numSymbols; ++i) {
        elf_symbol_t* elfSymbol = (elf_symbol_t*)(elf + symtab->offset + (i * symtab->entrySize));
        struct Symbol* symbol   = (struct Symbol*)(data + sizeof(struct SymbolData) + (sizeof(struct Symbol) * i));

        symbol->address = elfSymbol->addr;
        symbol->name    = elfSymbol->name;
    }

    bluescreen_symbols = header;
}
