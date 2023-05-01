#include <log.h>
#include <hpet.h>
#include <string.h>
#include <vm.h>

struct hpet_mmio {
    struct {
        uint64_t rev_id             : 8;

        uint64_t num_tim_cap        : 5;
        uint64_t count_size_cap     : 1;
        uint64_t _reserved          : 1;
        uint64_t leg_route_cap      : 1;

        uint64_t vendor_id          : 16;

        uint64_t counter_clk_period : 32;
    }__attribute__((packed)) capabilities;

    uint64_t _padding1;

    struct {
        uint64_t  enable_cnf : 1;
        uint64_t  leg_rt_cnf : 1;
        uint64_t  _reserved1 : 62;
    }__attribute__((packed)) configuration;

    uint64_t _padding2;

    struct {
        uint64_t  t0_int_sts : 1;
        uint64_t  t1_int_sts : 1;
        uint64_t  t2_int_sts : 1;
        uint64_t  _reserved1 : 61;
    }__attribute__((packed)) interrupt_status;

    uint64_t _padding3[25];

    volatile uint64_t main_counter_register;

    uint64_t _padding4;

    struct {
        struct {
            uint64_t _reserved1         : 1;
            uint64_t tn_int_type_cnf    : 1;
            uint64_t tn_int_enb_cnf     : 1;
            uint64_t tn_type_cnf        : 1;
            uint64_t tn_per_int_cnf     : 1;
            uint64_t tn_size_cap        : 1;
            uint64_t tn_val_set_cnf     : 1;
            uint64_t _reserved2         : 1;
            uint64_t tn_32mode_cnf      : 1;
            uint64_t tn_int_route_cnf   : 5;
            uint64_t tn_fsb_en_cnf      : 1;
            uint64_t tn_fsb_int_del_cap : 1;

            uint64_t _reserved3         : 16;

            uint64_t tn_int_route_cap   : 32;
        }__attribute__((packed)) config_and_caps;

        union {
            struct {
                uint64_t val32     : 32;
                uint64_t _reserved : 32;
            }__attribute__((packed));

            uint64_t val64;
        }__attribute__((packed)) comparator_value;

        struct {
            uint64_t tn_fsb_int_val  : 32;
            uint64_t tn_fsb_int_addr : 32;
        }__attribute__((packed)) fsb_interrupt_route;

        uint64_t _reserved;
    }__attribute__((packed)) timers[32];
}__attribute__((packed));

struct hpet_acpi_table {
    struct acpi_table_header header;

    uint8_t  hardware_revision;
    uint8_t  num_comparators             : 5;
    uint8_t  count_size_cap              : 1;
    uint8_t  _reserved                   : 1;
    uint8_t  legacy_replacement_irq_cap  : 1;
    uint16_t pci_vendor_id;

    struct acpi_address base_address;
    uint8_t             hpet_number;
    uint16_t            main_counter_minimum_clock_tick;

    uint8_t  page_protection : 4;
    uint8_t  oem             : 4;
}__attribute__((packed));

static volatile struct hpet_mmio* hpet;

static uint16_t ticks_to_ns_multiplier = 1;

void init_hpet(struct acpi_table_header* header) {
    struct hpet_acpi_table* table = (struct hpet_acpi_table*)header;

    uint64_t rev        = table->hardware_revision;
    uint64_t num_comp   = table->num_comparators;
    uint64_t pci_vendor = table->pci_vendor_id;

    if(table->base_address.address_space != acpi_address_space_memory) {
        loge("hpet", "HPET not in system memory address space is not yet implemented");
        return;
    }

    hpet = (struct hpet_mmio*)vm_context_find_free(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_SLAB_4K, 1);
    vm_context_map(VM_KERNEL_CONTEXT, (ptr_t)hpet, (ptr_t)table->base_address.address, 6);

    uint64_t period = hpet->capabilities.counter_clk_period;

    if(period < 1000000) {
        loge("hpet", "hpet timer periods below 1ns are not yet implemented");
        return;
    }

    ticks_to_ns_multiplier = period / 1000000;

    logd("hpet", "configuring HPET at %d:%x (%x): vendor %x, rev %d, #comparators %d, ns multiplier %d",
        table->base_address.address_space, table->base_address.address, hpet,
        pci_vendor, rev, num_comp, ticks_to_ns_multiplier
    );

    hpet->main_counter_register = 0;
    hpet->configuration.enable_cnf = 1;

    logi("hpet", "HPET initialized, we now know the time");
}

uint64_t hpet_ns_per_tick() {
    if(!hpet) return 0;

    return ticks_to_ns_multiplier;
}

uint64_t hpet_ticks() {
    if(!hpet) return 0;

    return hpet->main_counter_register;
}

void sc_handle_clock_read(uint64_t* nanoseconds) {
    *nanoseconds   = hpet_ticks() * hpet_ns_per_tick();
}
