#ifndef _CPU_H_INCLUDED
#define _CPU_H_INCLUDED

#include <log.h>
#include <stdint.h>

typedef struct {
    uint16_t fcw;
    uint16_t fsw;
    uint8_t  ftw;
    uint8_t  _reserved1;
    uint16_t fop;
    uint64_t fip;
    uint64_t fdp;
    uint32_t mxcsr;
    uint32_t mxcsr_mask;
    uint64_t mm0[2];
    uint64_t mm1[2];
    uint64_t mm2[2];
    uint64_t mm3[2];
    uint64_t mm4[2];
    uint64_t mm5[2];
    uint64_t mm6[2];
    uint64_t mm7[2];
    uint64_t xmm0[2];
    uint64_t xmm1[2];
    uint64_t xmm2[2];
    uint64_t xmm3[2];
    uint64_t xmm4[2];
    uint64_t xmm5[2];
    uint64_t xmm6[2];
    uint64_t xmm7[2];
    uint64_t xmm8[2];
    uint64_t xmm9[2];
    uint64_t xmm10[2];
    uint64_t xmm11[2];
    uint64_t xmm12[2];
    uint64_t xmm13[2];
    uint64_t xmm14[2];
    uint64_t xmm15[2];
    uint64_t _reserved2[6];
    uint64_t _available1[6];

    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t interrupt, error_code;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
}__attribute__((packed,aligned(16))) cpu_state;

#define DUMP_CPU(cpu)  \
    logd("cpudump", "<-- cut here [CPU DUMP START] ---->"); \
    logd("cpudump", "%3s: 0x%016x %3s: 0x%016x %3s: 0x%016x %7s: 0x%016x", "RAX", cpu->rax, "RBX", cpu->rbx, "RCX", cpu->rcx, "RDX",    cpu->rdx); \
    logd("cpudump", "%3s: 0x%016x %3s: 0x%016x %3s: 0x%016x %7s: 0x%016x", "RSI", cpu->rsi, "RDI", cpu->rdi, "RBP", cpu->rbp, "RSP",    cpu->rsp); \
    logd("cpudump", "%3s: 0x%016x %3s: 0x%016x %3s: 0x%016x %7s: 0x%016x", "R8",  cpu->r8,  "R9",  cpu->r9,  "R10", cpu->r10, "R11",    cpu->r11); \
    logd("cpudump", "%3s: 0x%016x %3s: 0x%016x %3s: 0x%016x %7s: 0x%016x", "R12", cpu->r12, "R13", cpu->r13, "R14", cpu->r14, "R15",    cpu->r15); \
    logd("cpudump", "%3s: 0x%016x %3s: 0x%016x %3s: 0x%016x %7s: 0x%016x", "RIP", cpu->rip, "CS",  cpu->cs,  "SS",  cpu->ss,  "RFLAGS", cpu->rflags); \
    logd("cpudump", "<-- cut here [CPU DUMP END] ---->"); \

#endif
