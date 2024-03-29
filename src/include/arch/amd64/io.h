#ifndef _IO_H_INCLUDED
#define _IO_H_INCLUDED

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1"::"a"(data), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t data) {
    asm volatile("outw %0, %1"::"a"(data), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t data) {
    asm volatile("outl %0, %1"::"a"(data), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0":"=a"(ret):"Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0":"=a"(ret):"Nd"(port));
    return ret;
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0":"=a"(ret):"Nd"(port));
    return ret;
}

#endif
