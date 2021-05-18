#include <lfostest.h>
#include <usb_dbc.c>

struct vm_table* VM_KERNEL_CONTEXT;
void vm_tlb_flush(uint64_t v) {
}

ptr_t vm_context_alloc_pages(struct vm_table* context, region_t region, size_t num) {
    return (ptr_t)malloc(num * 4096);
}

ptr_t vm_context_get_physical_for_virtual(struct vm_table* context, ptr_t v) {
    return v;
}

void* mm_alloc_pages(size_t num) {
    return malloc(num * 4096);
}

void vm_context_map(struct vm_table* context, ptr_t p, ptr_t vm, uint8_t pat) {
}

void* vm_alloc(size_t size) {
    void* data                = malloc(size + 16);
    *(uint64_t*)data          = size;
    *(uint64_t*)(data + size) = ~size;

    return data;
}

__attribute__ ((visibility ("default"))) void testmain(TestUtils* t) {
    union xhci_trb event;

    t->eq_size_t(sizeof(event.generic),             16, "XHCI generic event TRB");
    t->eq_size_t(sizeof(event.transfer_event),      16, "XHCI transfer event TRB");
    t->eq_size_t(sizeof(event.command_completion),  16, "XHCI command completion event TRB");
    t->eq_size_t(sizeof(event.port_status_change),  16, "XHCI port status change event TRB size");
    t->eq_size_t(sizeof(event.bandwidth_request),   16, "XHCI bandwidth request event TRB size");
    t->eq_size_t(sizeof(event.doorbell),            16, "XHCI doorbell event TRB size");
    t->eq_size_t(sizeof(event.host_controller),     16, "XHCI host controller event TRB size");
    t->eq_size_t(sizeof(event.device_notification), 16, "XHCI device notification event TRB size");
    t->eq_size_t(sizeof(event.mfindex_wrap),        16, "XHCI mfindex wrap TRB size");
    t->eq_size_t(sizeof(event.normal),              16, "XHCI normal TRB size");
    t->eq_size_t(sizeof(event.link),                16, "XHCI link TRB size");

    t->eq_size_t(sizeof(struct xhci_endpoint_context), 32, "XHCI endpoint");
    t->eq_size_t(sizeof(struct dbc_endpoint_context),  64, "DbC endpoint");

    t->eq_ptr_t((ptr_t)dbc_event_segment_table & 0xf, 0, "DbC event segment table alignment");
    t->eq_ptr_t((ptr_t)&dbc_context            & 0xf, 0, "DbC context alignment");

    t->eq_ptr_t((ptr_t)&dbc_language_str     & 0x1, 0, "DbC language string alignment");
    t->eq_ptr_t((ptr_t)&dbc_manufacturer_str & 0x1, 0, "DbC manufacturer string alignment");
    t->eq_ptr_t((ptr_t)&dbc_product_str      & 0x1, 0, "DbC product string alignment");
    t->eq_ptr_t((ptr_t)&dbc_serial_str       & 0x1, 0, "DbC serial string alignment");
}
