#include "bluescreen.h"
#include "config.h"
#include "string.h"

extern const char* LAST_INIT_STEP;

struct StackFrame {
    struct StackFrame* prev;
    ptr_t              rip;
};

void _panic_message(const char* message) {
    fbconsole_clear(0, 0, 127);
    fbconsole_write("\e[38;5;15m\e[48;5;4m");
    fbconsole_write("An error occured and LF OS has to be halted.\n"
                    "Below you can find more information:\n\n");

    fbconsole_write("LF OS build:    %s\n",   BUILD_ID);
    fbconsole_write("Last init step: %s\n",   LAST_INIT_STEP);
    fbconsole_write("Error message:  %s\n\n", message);

    fbconsole_write("\nStack trace that lead to this error:\n");

    struct StackFrame* frame;
    asm("mov %%rbp,%0":"=r"(frame));

    while(frame && (ptr_t)frame >= 0xFFFF800000000000) {
        fbconsole_write("  -> 0x%016x\n", frame->rip);
        frame = frame->prev;
    }

    fbconsole_write("\n\n");
}

void panic() {
    panic_message("Unknown error");
}

void panic_message(const char* message) {
    _panic_message(message);
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

    char message[256];
    ksnprintf(message, 256, "Interrupt: 0x%02x (%s), error: 0x%04x", cpu->interrupt, exceptions[cpu->interrupt], cpu->error_code);
    _panic_message(message);

    DUMP_CPU(cpu);

    while(1);
}
