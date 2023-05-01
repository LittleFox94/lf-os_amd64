#include <stdint.h>
#include <log.h>
#include <vm.h>

typedef enum {
    DESTINATION_MODE_PHYSICAL = 0,
    DESTINATION_MODE_LOGICAL  = 1,
} DestinationMode;

typedef enum {
    POLARITY_HIGH_ACTIVE = 0,
    POLARITY_LOW_ACTIVE  = 1,
} Polarity;

typedef enum {
    TRIGGER_EDGE  = 0,
    TRIGGER_LEVEL = 1,
} Trigger;

struct IOAPICRedirection {
    uint8_t vector                  : 8;
    uint8_t delivery_mode           : 3;
    DestinationMode destination_mode: 1;
    bool delivery_status            : 1;
    Polarity polarity               : 1;
    bool remote_irr                 : 1;
    Trigger trigger_mode            : 1;
    bool masked                     : 1;
    uint64_t _reserved              : 39;
    uint8_t destination             : 8;
}__attribute__((packed));

struct IOAPIC {
    volatile uint32_t address;
    uint32_t _padding[3];
    volatile uint32_t data;
}__attribute__((packed));

static volatile struct IOAPIC* ioapic;

static inline uint32_t ioapic_read(uint32_t reg) {
    ioapic->address = reg;
    return ioapic->data;
}

static inline void ioapic_write(uint32_t reg, uint32_t data) {
    ioapic->address = reg;
    ioapic->data = data;
}

static inline void ioapic_set_redir(
    uint64_t idx, uint8_t vector, DestinationMode dm, Polarity pol, Trigger trig,
    bool masked, uint8_t destination
) {
    union {
        struct IOAPICRedirection redir;
        uint64_t u64;
    } redir = { .redir = {
        .vector             = vector,
        .delivery_mode      = 0,
        .destination_mode   = dm,
        .polarity           = pol,
        .trigger_mode       = trig,
        .masked             = masked,
        .destination        = destination
    } };

    ioapic_write(0x10 + (idx * 2), (uint32_t)(redir.u64 & 0xFFFFFFFF));
    ioapic_write(0x11 + (idx * 2), (uint32_t)(redir.u64 >> 32));
}

/**
 * Initialize I/O APIC at the given physical base address.
 *
 * \param base Physical base address of the I/O APIC to initialize
 *
 * \remarks Even though this function looks like it can be invoked multiple times,
 *          it's currently only built to initialize one I/O APIC - support for multiple
 *          I/O APICs is left for the future.
 */
void init_io_apic(void* base) {
    ioapic = (volatile struct IOAPIC*)vm_context_find_free(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, 1);
    vm_context_map(VM_KERNEL_CONTEXT, (ptr_t)ioapic, (ptr_t)base, 6);

    uint64_t id = ioapic_read(0);
    logi("apic", "Initializing I/O APIC 0x%x at 0x%x (0x%x)", id, base, ioapic);

    uint32_t ver_reg = ioapic_read(1);
    uint64_t version = ver_reg & 0xFF;
    uint64_t redirs  = ((ver_reg >> 16) & 0xFF) + 1;

    logd("apic", "I/O APIC %d version 0x%x with %d redirection entries", id, version, redirs);

    if(redirs > 16) {
        logi("apic", "Only using the first 16 redirections for now");
        redirs = 16;
    }

    // for now we just initalize all redirections to trigger interrupt 0x20 + redirection index

    for(uint64_t redir = 0; redir < redirs; ++redir) {
        ioapic_set_redir(
            redir,
            0x20 + redir,   // vector
            DESTINATION_MODE_PHYSICAL,
            POLARITY_HIGH_ACTIVE,
            TRIGGER_EDGE,
            false,          // Masked
            0               // Destination - Local APIC ID
        );
    }
}
