#ifndef _BLUESCREEN_H_INCLUDED
#define _BLUESCREEN_H_INCLUDED

#include "cpu.h"

__attribute__((noreturn)) void panic();
__attribute__((noreturn)) void panic_message(const char* message);

__attribute__((noreturn)) void panic_cpu(const cpu_state* cpu);

#endif
