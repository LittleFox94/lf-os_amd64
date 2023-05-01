#include "apic_internals.h"

#include <log.h>
#include <hpet.h>
#include <msr.h>
#include <cpuid.h>
#include <bluescreen.h>

void init_lapic_timer(bool integrated) {
    // due to a Windows NT bug, we first have to enable access to some CPUID levels
    // ref notes on https://sandpile.org/x86/cpuid.htm#level_0000_0015h
    uint64_t msr_misc_enable = read_msr(0x1A0);
    msr_misc_enable &= ~(1 << 21); // LCMV
    write_msr(0x1A0, msr_misc_enable);

    // retrieve LAPIC timer frequency
    uint64_t frequency = 0;
    if(integrated) {
        // retrieve core crystal frequency
        cpuid(0x15, 0, 0, &frequency, 0);
    }
    else {
        // retrieve bus frequency and convert to Hz
        cpuid(0x16, 0, 0, &frequency, 0);
        frequency *= 1000000;
    }

    if(!frequency) {
        // this is sadly the default case ...
        // calibrate via already initialized HPET
        uint64_t ns_per_tick = hpet_ns_per_tick();
        uint64_t ticks       = hpet_ticks();

        uint32_t initial = -1;
        lapic_write(InitialCount, initial);

        // wait 1µs
        while(hpet_ticks() < ticks + (1000000000ULL / ns_per_tick));

        uint32_t final = lapic_read(CurrentCount);
        uint64_t counts = initial - final;
        frequency = counts;
        lapic_write(InitialCount, 0);
    }

    logd("apic", "Found LAPIC timer frequency to be %u", frequency);

    // we want an interrupt every 1ms
    const uint64_t target_interrupt_frequency = 1000;

    uint8_t divisor     = 1;
    uint8_t divisor_reg = 7;
    uint64_t period     = 0;

    do {
        if(divisor == 128) {
            panic_message("LAPIC timer: can't find divisor allowing our target interrupt frequency!");
        }

        // only increase after first iteration
        if(period) {
            if(divisor_reg == 7) {
                divisor_reg = 0;
            }

            divisor *= 2;
            ++divisor_reg;
        }

        period = (frequency / divisor) / target_interrupt_frequency;
    } while(period > 0xFFFFFFFF);

    // somehow there are three bits to select the divisor, but they choose bits 0, 1 and 3 for that?!
    uint32_t divisor_reg_wtf_intel = (divisor_reg & 3) | (divisor_reg & 4 ? 8 : 0);

    logd(
        "apic", "Configuring LAPIC timer for divisor %u (reg index %x, actual reg %x) and period %u",
        (uint64_t)divisor, (uint64_t)divisor_reg, (uint64_t)divisor_reg_wtf_intel, period
    );

    lapic_write(DivideConfiguration, divisor_reg_wtf_intel);

    lapic_lvt_register lvt_timer;
    lvt_timer.u32 = lapic_read(LVTTimer);
    lvt_timer.s.vector     = 0x20;
    lvt_timer.s.masked     = 0;
    lvt_timer.s.timer_mode = 0x01;
    lapic_write(LVTTimer, lvt_timer.u32);

    lapic_write(InitialCount, period);
}
