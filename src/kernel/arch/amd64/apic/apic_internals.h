#ifndef _APIC_INTERNALS_H_INCLUDED
#define _APIC_INTERNALS_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ID = 0x20,
    Version = 0x30,
    TaskPriority = 0x80,
    ArbitrationPriority = 0x90,
    ProcessorPriority = 0xA0,
    EOI = 0xB0,
    RemoteRead = 0xC0,
    LogicalDestination = 0xD0,
    DestinationFormat = 0xE0,
    SpuriousInterruptVector = 0xF0,
    // TODO: In-Service
    // TODO: Trigger Mode
    // TODO: Interrupt Request
    ErrorStatus = 0x280,
    LVCCorrectedMachineCheckInterrupt = 0x2F0,
    InterruptCommand = 0x300,
    InterruptCommandDestination = 0x310,
    LVTTimer = 0x320,
    LVTThermalSensor = 0x330,
    LVTPerformanceMonitoringCounters = 0x340,
    LVTLINT0 = 0x350,
    LVTLINT1 = 0x360,
    LVTError = 0x370,
    InitialCount = 0x380,
    CurrentCount = 0x390,
    DivideConfiguration = 0x3E0,
} lapic_register;

typedef union {
    uint32_t u32;
    struct {
        uint8_t vector          : 8;
        uint8_t delivery_mode   : 3;
        uint8_t _reserved0      : 1;
        uint8_t delivery_status : 1;
        uint8_t polarity        : 1;
        uint8_t remote_irr      : 1;
        uint8_t trigger_mode    : 1;
        uint8_t masked          : 1;
        uint8_t timer_mode      : 3;
        uint16_t _reserved1     : 12;
    }__attribute__((packed)) s;
} lapic_lvt_register;

extern void* lapic_base;

static inline uint32_t lapic_read(lapic_register reg) {
    volatile uint32_t* addr = lapic_base + reg;
    return *addr;
}

static inline void lapic_write(lapic_register reg, uint32_t val) {
    volatile uint32_t* addr = lapic_base + reg;
    *addr = val;
}

void init_legacy_pic();
void init_lapic_timer(bool integrated);

#endif
