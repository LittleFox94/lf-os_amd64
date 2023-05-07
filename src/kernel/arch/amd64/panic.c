#include "panic.h"
#include "config.h"
#include "string.h"
#include "fbconsole.h"
#include "elf.h"
#include "qr.h"

extern const char* LAST_INIT_STEP;

struct StackFrame {
    struct StackFrame* prev;
    ptr_t              rip;
};

void* kernel_symbols = 0;

// \cond panic_functions
static void panic_message_impl(const char* message, uint64_t rbp, bool rbp_given) {
    //fbconsole_clear(0, 0, 0);
    logf("panic", "An error occured and LF OS has to be halted. More info below:");

    logf("panic", "LF OS build:    %s",   BUILD_ID);
    logf("panic", "Last init step: %s",   LAST_INIT_STEP);
    logf("panic", "Error message:  %s\n", message);

    struct StackFrame* frame;

    if(!rbp_given) {
        asm("mov %%rbp,%0":"=r"(frame));
    }
    else {
        frame = (struct StackFrame*)rbp;
    }

    if(frame) {
        logi("panic", "Stack trace that led to this error:");

        int i = 20;
        while(frame && frame->rip > 0xFFFF800000000000 && --i) {
            char symbol[256];
            size_t symbol_size = sizeof(symbol);
            if(!elf_symbolize(kernel_symbols, frame->rip, &symbol_size, symbol)) {
                symbol[0] = 0;
            }

            logi("panic", "  0x%016x %s", frame->rip, symbol);

            if(frame->prev != frame) {
                frame = frame->prev;
            }
            else {
                frame = 0;
            }
        }
    }
}

static void panic_qr() {
    qr_data qr;
    uint8_t modules = qr_log(qr);

    uint8_t px_per_module = 4; // squared! 4 meaning 4x4

    uint32_t img[(modules + 8)*(modules + 8)*px_per_module*px_per_module]; // 177x177 modules from qr, adding 4 modules quiet area on all sides
    memset(img, 0xFF, sizeof(img));

    for(uint8_t y = 0; y < modules; ++y) {
        for(uint8_t x = 0; x < modules; ++x) {
            uint8_t d = qr[(y*177)+x];
            if(d & 1) {
                for(uint8_t iy = 0; iy < px_per_module; ++iy) {
                    for(uint8_t ix = 0; ix < px_per_module; ++ix) {
                        uint16_t fx = (x * px_per_module) + ix + (4 * px_per_module);
                        uint16_t fy = (y * px_per_module) + iy + (4 * px_per_module);
                        img[(fy * (modules + 8) * px_per_module) + fx] = 0x00000000;
                    }
                }
            }
        }
    }

    uint16_t img_size = (modules + 8) * px_per_module;

    fbconsole_blt((uint8_t*)img, img_size, img_size, -img_size, -img_size);
}
// \endcond

void panic(void) {
    // \cond panic_functions
    panic_message("Unknown error");
    // \endcond
}

void panic_message(const char* message) {
    // \cond panic_functions
    panic_message_impl(message, 0, false);

    panic_qr();

    while(1) {
        asm("hlt");
    }
    // \endcond
}

void panic_cpu(const cpu_state* cpu) {
    // \cond panic_functions
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

        char symbol[256];
        size_t symbol_size = sizeof(symbol);
        if(elf_symbolize(kernel_symbols, cr2, &symbol_size, symbol)) {
            ksnprintf(cr2_msg, 256, " @ 0x%016x, %s", cr2, symbol);
        }
        else {
            ksnprintf(cr2_msg, 256, " @ 0x%016x", cr2);
        }

    }

    char message[256];
    ksnprintf(message, 256, "Interrupt: 0x%02x (%s), error: 0x%04x%s", cpu->interrupt, exceptions[cpu->interrupt], cpu->error_code, cr2_msg);
    panic_message_impl(message, cpu->rbp, true);

    char symbol[256];
    size_t symbol_size = sizeof(symbol);
    if(!elf_symbolize(kernel_symbols, cpu->rip, &symbol_size, symbol)) {
        symbol[0] = 0;
    }

    logi("panic", "RIP: 0x%016x %s", cpu->rip, symbol);

    DUMP_CPU(cpu);

    panic_qr();

    while(1) {
        asm("hlt");
    }
    // \endcond
}

