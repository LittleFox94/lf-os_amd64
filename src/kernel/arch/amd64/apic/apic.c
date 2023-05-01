#include "apic_internals.h"

#include <log.h>
#include <msr.h>
#include <vm.h>

void* lapic_base;

void init_io_apic(void* base);

void init_apic() {
    init_legacy_pic();
    logd("apic", "Legacy PIC configured");

    uint32_t base_register = read_msr(0x1B);
    ptr_t base = base_register & 0xFFFFF000;
    logd("apic", "Physical base address is %x", base);

    lapic_base = (void*)vm_context_find_free(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, 1);
    logd("apic", "Virtual base address will be %x", lapic_base);

    vm_context_map(VM_KERNEL_CONTEXT, (ptr_t)lapic_base, (ptr_t)base, 6);
    logd("apic", "Virtual base address is %x", lapic_base);

    uint64_t lapic_id         = lapic_read(ID);
    logd("apic", "reading ID success", lapic_base);
    uint64_t lapic_version    = lapic_read(Version) & 0xFF;
    logd("apic", "reading vesion success", lapic_base);
    bool     lapic_integrated = !!(lapic_version & 0xF0);

    logd(
        "apic", "Initializing LAPIC %u at %x (version %x, %s)",
        lapic_id, base, lapic_version, lapic_integrated ? "integrated" : "discrete"
    );

    base_register |= 0x800;
    write_msr(0x1B, base_register);

    uint32_t spurious = lapic_read(SpuriousInterruptVector);
    spurious |= 0x100;
    lapic_write(SpuriousInterruptVector, spurious);

    init_lapic_timer(lapic_integrated);

    init_io_apic((void*)0xFEC00000);
}

void apic_set_handled(int interrupt) {
    uint32_t lvt = lapic_read(LVTTimer);
    lvt &= ~0x10000;
    lapic_write(LVTTimer, lvt);

    lapic_write(EOI, interrupt);
}
