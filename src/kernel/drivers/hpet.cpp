#include <log.h>
#include <hpet.h>
#include <string.h>
#include <vm.h>

#include <stddef.h>

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

    uint64_t main_counter_register;

    uint64_t _padding4;

    struct {
        struct {
            uint64_t _reserved1         : 1;
            uint64_t tn_int_type_cnf    : 1;
            uint64_t tn_int_enb_cnf     : 1;
            uint64_t tn_type_cnf        : 1;
            uint64_t tn_per_int_cap     : 1;
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
            }__attribute__((packed)) b32;

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

static struct hpet_mmio* volatile hpet;

static uint64_t initialization_ticks = 0;
static uint16_t ticks_to_ns_multiplier = 1;

static void dump_hpet_caps(struct hpet_mmio* hpet) {
    logi(
        "hpet", "capabilities: rev_id=0x%x, num_tim_cap=%d, count_size_cap=%d, leg_route_cap=%d, vendor_id=0x%04x, counter_clk_period=%d",
        (uint64_t)hpet->capabilities.rev_id,
        (uint64_t)hpet->capabilities.num_tim_cap,
        (uint64_t)hpet->capabilities.count_size_cap,
        (uint64_t)hpet->capabilities.leg_route_cap,
        (uint64_t)hpet->capabilities.vendor_id,
        (uint64_t)hpet->capabilities.counter_clk_period
    );
}

static void dump_timer_caps(struct hpet_mmio* hpet, unsigned timer) {
    logi(
        "hpet", "timer %d capabilities: tn_per_int_cap=0x%x, tn_size_cap=0x%x, tn_fsb_int_del_cap=0x%x, tn_int_route_cap=0x%x",
        timer,
        (uint64_t)hpet->timers[timer].config_and_caps.tn_per_int_cap,
        (uint64_t)hpet->timers[timer].config_and_caps.tn_size_cap,
        (uint64_t)hpet->timers[timer].config_and_caps.tn_fsb_int_del_cap,
        (uint64_t)hpet->timers[timer].config_and_caps.tn_int_route_cap
    );
}

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
    vm_context_map(VM_KERNEL_CONTEXT, (uint64_t)hpet, (uint64_t)table->base_address.address, 6);

    uint64_t period = hpet->capabilities.counter_clk_period;

    if(period < 1000000) {
        loge("hpet", "hpet timer periods below 1ns are not yet implemented");
        return;
    }

    ticks_to_ns_multiplier = period / 1000000;

    dump_hpet_caps(hpet);
    for(unsigned i = 0; i < hpet->capabilities.num_tim_cap; ++i) {
        dump_timer_caps(hpet, i);
    }

    logd("hpet", "configuring HPET at %d:%x (%x): vendor %x, rev %d, #comparators %d, ns multiplier %d",
        table->base_address.address_space, table->base_address.address, hpet,
        pci_vendor, rev, num_comp, ticks_to_ns_multiplier
    );

    auto configuration = hpet->configuration;
    configuration.enable_cnf = 0;
    configuration.leg_rt_cnf = 1;
    hpet->configuration = configuration;
    hpet->main_counter_register = 0;

    auto config_and_caps = hpet->timers[0].config_and_caps;
    config_and_caps.tn_int_enb_cnf = 1;
    config_and_caps.tn_type_cnf = 1;
    config_and_caps.tn_fsb_en_cnf = 0;
    config_and_caps.tn_val_set_cnf = 1;

    hpet->timers[0].config_and_caps = config_and_caps;
    hpet->timers[0].comparator_value.val64 = 0;
    hpet->timers[0].comparator_value.val64 = 100000 * ticks_to_ns_multiplier;

    configuration.enable_cnf = 1;
    hpet->configuration = configuration;

    initialization_ticks = hpet->main_counter_register;

    logd("hpet", "initialization_ticks = %u", initialization_ticks);
    logi("hpet", "HPET initialized, we now know the time");
}

void sc_handle_clock_read(uint64_t* nanoseconds) {
    uint64_t ticks = hpet->main_counter_register -
                     initialization_ticks;
    *nanoseconds   = ticks * ticks_to_ns_multiplier;
}
