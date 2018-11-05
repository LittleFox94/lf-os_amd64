#include "msr.h"

void write_msr(uint32_t msr, uint64_t value) {
    asm volatile(
        "wrmsr" ::
        "a"(value & 0xFFFFFFFF), "d"(value >> 32), "c"(msr)
    );
}

uint64_t read_msr(uint32_t msr) {
    uint64_t lo, hi;
    asm volatile(
        "rdmsr"            :
        "=a"(lo), "=d"(hi) :
        "c"(msr)
    ); 

    return (hi << 32) | lo;
}
