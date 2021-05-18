/**
 * nearly standalone USB xHCI DbC driver
 *
 * includes:
 *  - PC PCI enumeration
 *  - xHCI DbC initialization
 *  - USB descriptors
 *  - USB device code
 *
 * requires:
 *  - some standard headers and functions
 */

#include <stdint.h>
#include <stdbool.h>
#include <cpu.h>

#include <log.h>
#include <mm.h>
#include <vm.h>
#include <string.h>

#include <usb_dbc.h>

#define write32(address, val) asm("movl %1, %0\n\tmfence"::"m"(address), "r"(*(uint32_t*)&val):"memory");
#define read32(address, val) asm("mfence\n\tmovl %0, %1"::"m"(address), "r"(*(uint32_t*)val):"memory");

static const int PCI_CONFIG_ADDRESS_PORT = 0xCF8;
static const int PCI_CONFIG_DATA_PORT    = 0xCFC;

union pci_config_addr {
    struct {
        uint8_t reg_offset: 8;
        uint8_t function:   3;
        uint8_t device:     5;
        uint8_t bus:        8;
        uint8_t _reserved:  7;
        bool enable:        1;
    } __attribute__((packed));

    uint32_t addr;
};

union pci_config_class_reg {
    struct {
        uint8_t rev;
        uint8_t interface;
        uint8_t subclass;
        uint8_t class;
    } __attribute__((packed));

    uint32_t reg;
};

//! Checks if the given device is an XHCI controller, returns the BAR0 if it is
static uint64_t xhci_pci_bar(uint8_t bus, uint8_t device, uint8_t func) {
    const union pci_config_addr class = {
        .reg_offset = 0x08,
        .function   = func,
        .device     = device,
        .bus        = bus,
        .enable     = true,
    };

    outl(PCI_CONFIG_ADDRESS_PORT, class.addr);
    union pci_config_class_reg class_val = { .reg = inl(0xCFC) };

    if(!~class_val.reg) {
        // no device here
        return 0;
    }

    if(class_val.class     != XHCI_PCI_CLASS ||
       class_val.subclass  != XHCI_PCI_SUBCLASS ||
       class_val.interface != XHCI_PCI_INTERFACE) {
        // not an XHCI controller
        return 0;
    }

    const union pci_config_addr bar0 = {
        .reg_offset = 0x10,
        .function   = func,
        .device     = device,
        .bus        = bus,
        .enable     = true,
    };
    outl(PCI_CONFIG_ADDRESS_PORT, bar0.addr);
    uint32_t bar0_val = inl(PCI_CONFIG_DATA_PORT);

    const union pci_config_addr bar1 = {
        .reg_offset = 0x14,
        .function   = func,
        .device     = device,
        .bus        = bus,
        .enable     = true,
    };
    outl(PCI_CONFIG_ADDRESS_PORT, bar1.addr);
    uint32_t bar1_val = inl(PCI_CONFIG_DATA_PORT);

    uint64_t bar = ((uint64_t)bar1_val << 32) | bar0_val;

    // check for 64bit BAR and return that cleaned
    if((bar & 7) == 4) {
        return bar & ~0xf;
    } else {
        return 0;
    }
}

static void xhci_dbc_detect(uint64_t bar);

void init_usb_dbc() {
    for(uint8_t bus = 0; bus < 0xFF; ++bus) {
        for(uint8_t dev = 0; dev < 0x1F; ++dev) {
            for(uint8_t func = 0; func < 0x08; ++func) {
                uint64_t bar = xhci_pci_bar(bus, dev, func);

                if(bar) {
                    logi("usb_dbc", "Found XHCI controller at 0x%x", bar);
                    xhci_dbc_detect(bar);
                }
            }
        }
    }
}

static void xhci_dbc_init(uint64_t bar, uint64_t excap);

void xhci_dbc_detect(uint64_t bar) {
    ptr_t mmio_space = vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_SLAB_4K, 1);
    vm_context_map(VM_KERNEL_CONTEXT, mmio_space, bar, 0x03);

    struct xhci_capabilities* cap = (struct xhci_capabilities*)(mmio_space);

    logd("usb_dbc", "XHCI controller at 0x%x has cap length of %d",
            bar, cap->caplength, (uint64_t)cap->hciversion);

    uint32_t excap_off = (cap->hccparams1 >> 16) << 2; // shift to "only excap offset", then multiply with 4
    logd("usb_dbc", "Extended capabilities starting at 0x%x",
            bar + excap_off);

    ptr_t excap_virtual = vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_SLAB_4K, 1);

    do {
        vm_context_map(VM_KERNEL_CONTEXT, excap_virtual, bar + excap_off, 0x03);
        vm_tlb_flush(excap_virtual);

        struct xhci_extended_capabilities* excap = (struct xhci_extended_capabilities*)(excap_virtual + (excap_off & 0xFFF));

        if(excap->cap_id == XHCI_EXTENDED_CAPABILITY_LEGACY) {
            struct xhci_legacy_capability* legacy_cap = (struct xhci_legacy_capability*)excap;
            logi("usb_dbc", "XHCI controller has legacy capability at 0x%x", legacy_cap);

            if(legacy_cap->hc_bios_owned) {
                logd("usb_dbc", "HC owned by firmware, let's change that");

                struct xhci_legacy_capability l = *legacy_cap;
                l.hc_bios_owned = 0;
                l.hc_os_owned   = 1;

                write32(legacy_cap, l);
                read32(legacy_cap, &l);

                while(l.hc_bios_owned) {
                    read32(legacy_cap, &l);
                    logd("usb_dbc", "XHCI controller still owned by firmware");
                }

                logw("usb_dbc", "HC now in OS control");
            }
            else {
                logd("usb_dbc", "HC not owned by firmware");
            }
        }
        else if(excap->cap_id == XHCI_EXTENDED_CAPABILITY_DEBUG) {
            logi("usb_dbc", "XHCI controller has debug capability \\o/");
            xhci_dbc_init(bar, (uint64_t)excap);
        }

        if(excap->next) {
            excap_off += excap->next << 2;
        }
        else {
            break;
        }
    } while(excap_off);
}

__attribute__((aligned(16)))
static struct xhci_event_ring_segment_table_entry dbc_event_segment_table[1];

__attribute__((aligned(2)))
static const char* dbc_language_str = "en-us";

__attribute__((aligned(2)))
static const char* dbc_manufacturer_str = "Mara Sophie Grosch & LF OS contributors";

__attribute__((aligned(2)))
static const char* dbc_product_str = "LF OS XHCI kernel debug";

__attribute__((aligned(2)))
static const char* dbc_serial_str = "1337";

__attribute__((aligned(16)))
static struct dbc_context dbc_context = {
    .out_endpoint_context = {
        .xhci = {
            // DbC does not support streams, so these are "reserved" and have to be 0
            .max_pstreams = 0,
            .lsa = 0,
            .hid = 0,

            // DbC endpoints are bulk, so these are "reserved" and have to be 0
            .interval = 0,
            .mult     = 0,
            .max_esit_payload_hi = 0,
            .max_esit_payload_lo = 0,

            // DbC says "software shall initialize the fields of the Endpoint Context as follows:"
            // so we do, even though I don't normally follow the rules
            .max_packet_size = 1024,
            // max_burst_size is initialized dynamically
            .ep_type = 2,
            .average_trb_length = 1024,
        },
    },
    .in_endpoint_context = {
        .xhci = {
            // DbC does not support streams, so these are "reserved" and have to be 0
            .max_pstreams = 0,
            .lsa = 0,
            .hid = 0,

            // DbC endpoints are bulk, so these are "reserved" and have to be 0
            .interval = 0,
            .mult     = 0,
            .max_esit_payload_hi = 0,
            .max_esit_payload_lo = 0,

            // DbC says "software shall initialize the fields of the Endpoint Context as follows:"
            // so we do, even though I don't normally follow the rules
            .max_packet_size = 1024,
            // max_burst_size is initialized dynamically
            .ep_type = 6,
            .average_trb_length = 1024,
        },
    },
};

static union xhci_trb *dbc_endpoint_trbs;

static void xhci_dbc_alloc_event_segment() {
    // XXX: +8 because of vm_alloc implementation detail:
    // allocated is the amount + 16, size is stored before the returned pointer
    // and ~size after the the given length to check for overflows
    const size_t event_segment_size = 254;
    struct dbc_event_segment* event_segment = vm_alloc(event_segment_size * sizeof(union xhci_trb)) + 8;

    dbc_event_segment_table[0].ring_segment = (ptr_t)event_segment >> 4;
    dbc_event_segment_table[0].ring_size    = event_segment_size;
}

static void xhci_dbc_alloc_ep_segments() {
    const size_t ep_segment_size = 127; // TRBs per EP
    dbc_endpoint_trbs = vm_alloc(ep_segment_size * sizeof(union xhci_trb) * 2) + 8;

    ptr_t page;
    size_t len_per_trb = 1024;
    size_t trb_per_page = 4096 / len_per_trb;

    // XXX: -1: last TRB is a link to start of the chain to make it a ring,
    // here we prepare the normal TRBs only
    for(size_t i = 0; i < ep_segment_size - 1; ++i) {
        if(i % (trb_per_page / 2) == 0) {
            page = (ptr_t)mm_alloc_pages(1);
        }

        ptr_t data_buffer = page
                          + (i % (trb_per_page / 2) * len_per_trb);

        // out trb
        dbc_endpoint_trbs[i] = (union xhci_trb){
            .normal = {
                .data_ptr        = data_buffer >> 4,
                .transfer_length = len_per_trb,

                .cycle    = 1,
                .ent      = 0,
                .isp      = 0,
                .no_snoop = 0,
                .chain    = 0,
                .ioc      = 0,
                .idt      = 0,
                .bei      = 0,
                .trb_type = XHCI_TRB_TYPE_NORMAL,
            }
        };

        // in trb
        dbc_endpoint_trbs[ep_segment_size + 1 + i] = (union xhci_trb){
            .normal = {
                .data_ptr        = (data_buffer + len_per_trb) >> 4,
                .transfer_length = len_per_trb,

                .cycle    = 0,
                .ent      = 0,
                .isp      = 0,
                .no_snoop = 0,
                .chain    = 0,
                .ioc      = 0,
                .idt      = 0,
                .bei      = 0,
                .trb_type = XHCI_TRB_TYPE_NORMAL,
            }
        };
    }

    // out link trb
    dbc_endpoint_trbs[ep_segment_size - 1] = (union xhci_trb){
        .link = {
            .segment_pointer    = (ptr_t)&dbc_endpoint_trbs[0] >> 4,
            .interruptor_target = 0,

            .cycle        = 1,
            .toggle_cycle = 1,
            .chain        = 0,
            .ioc          = 0,
            .trb_type     = XHCI_TRB_TYPE_LINK,
        }
    };

    // out link trb
    dbc_endpoint_trbs[(ep_segment_size * 2) - 1] = (union xhci_trb){
        .link = {
            .segment_pointer    = (ptr_t)&dbc_endpoint_trbs[ep_segment_size] >> 4,
            .interruptor_target = 0,

            .cycle        = 1,
            .toggle_cycle = 1,
            .chain        = 0,
            .ioc          = 0,
            .trb_type     = XHCI_TRB_TYPE_LINK,
        }
    };

    dbc_context.out_endpoint_context.xhci.tr_dequeue_pointer = (ptr_t)&dbc_endpoint_trbs[0] >> 4;
    dbc_context.in_endpoint_context.xhci.tr_dequeue_pointer  = (ptr_t)&dbc_endpoint_trbs[ep_segment_size] >> 4;
}

void xhci_dbc_init(uint64_t bar, uint64_t excap) {
    volatile struct xhci_debug_capability* dbc = (struct xhci_debug_capability*)excap;

    xhci_dbc_alloc_event_segment();
    xhci_dbc_alloc_ep_segments();

    dbc->dcerstsz.erst_size = 1;
    dbc->dcerstba.dcerstba  = vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, (ptr_t)dbc_event_segment_table) >> 4;
    dbc->dcerdp.dequeue_ptr = dbc->dcerstba.dcerstba;

    dbc_context.info_data = (struct dbc_info_context_data){
        .language_string_ptr    = (ptr_t)dbc_language_str >> 1,
        .language_string_length = strlen(dbc_language_str),

        .manufacturer_string_ptr    = (ptr_t)dbc_manufacturer_str >> 1,
        .manufacturer_string_length = strlen(dbc_manufacturer_str),

        .product_string_ptr    = (ptr_t)dbc_product_str >> 1,
        .product_string_length = strlen(dbc_product_str),

        .serial_string_ptr    = (ptr_t)dbc_serial_str >> 1,
        .serial_string_length = strlen(dbc_serial_str),
    };

    dbc_context.out_endpoint_context.xhci.max_burst_size = dbc->dcctrl.debug_max_burst_size;
    dbc_context.in_endpoint_context.xhci.max_burst_size  = dbc->dcctrl.debug_max_burst_size;

    dbc->dccd.vendor_id  = 0x1337;
    dbc->dccd.product_id = 0x1F05;
    dbc->dccd.revision   = 0x0010;

    dbc->dcctrl.dce = 1;
}
