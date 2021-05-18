#ifndef _USB_DBC_H_INCLUDED
#define _USB_DBC_H_INCLUDED

#include <stdint.h>

static const int XHCI_PCI_CLASS     = 0x0C;
static const int XHCI_PCI_SUBCLASS  = 0x03;
static const int XHCI_PCI_INTERFACE = 0x30;

static const int XHCI_EXTENDED_CAPABILITY_LEGACY = 1;
static const int XHCI_EXTENDED_CAPABILITY_DEBUG  = 10;

enum xhci_trb_type {
    XHCI_TRB_TYPE_RESERVED = 0,
    XHCI_TRB_TYPE_NORMAL,
    XHCI_TRB_TYPE_SETUP_STAGE,
    XHCI_TRB_TYPE_DATA_STAGE,
    XHCI_TRB_TYPE_STATUS_STAGE,
    XHCI_TRB_TYPE_ISOCH,
    XHCI_TRB_TYPE_LINK,
    XHCI_TRB_TYPE_EVENT_DATA,
    XHCI_TRB_TYPE_NO_OP,

    XHCI_TRB_TYPE_ENABLE_SLOT_COMMAND,
    XHCI_TRB_TYPE_DISABLE_SLOT_COMMAND,
    XHCI_TRB_TYPE_ADDRESS_DEVICE_COMMAND,
    XHCI_TRB_TYPE_CONFIGURE_ENDPOINT_COMMAND,
    XHCI_TRB_TYPE_EVALUATE_CONTEXT_COMMAND,
    XHCI_TRB_TYPE_RESET_ENDPOINT_COMMAND,
    XHCI_TRB_TYPE_STOP_ENDPOINT_COMMAND,
    XHCI_TRB_TYPE_SET_TR_DEQUEUE_POINTER_COMMAND,
    XHCI_TRB_TYPE_RESET_DEVICE_COMMAND,
    XHCI_TRB_TYPE_FORCE_EVENT_COMMAND,
    XHCI_TRB_TYPE_NEGOTIATE_BANDWIDTH_COMMAND,
    XHCI_TRB_TYPE_SET_LATENCY_TOLERANCE_VALUE_COMMAND,
    XHCI_TRB_TYPE_GET_PORT_BANDWIDTH_COMMAND,
    XHCI_TRB_TYPE_FORCE_HEADER_COMMAND,
    XHCI_TRB_TYPE_NO_OP_COMMAND,
    XHCI_TRB_TYPE_GET_EXTENDED_PROPERTY_COMMAND,
    XHCI_TRB_TYPE_SET_EXTENDED_PROPERTY_COMMAND,

    XHCI_TRB_TYPE_TRANSFER_EVENT = 32,
    XHCI_TRB_TYPE_COMMAND_COMPLETION_EVENT,
    XHCI_TRB_TYPE_PORT_STATUS_CHANGE_EVENT,
    XHCI_TRB_TYPE_BANDWIDTH_REQUEST_EVENT,
    XHCI_TRB_TYPE_DOORBELL_EVENT,
    XHCI_TRB_TYPE_HOST_CONTROLLER_EVENT,
    XHCI_TRB_TYPE_DEVICE_NOTIFICATION_EVENT,
    XHCI_TRB_TYPE_MFINDEX_WRAP_EVENT,
};

struct xhci_capabilities {
    uint8_t caplength;
    uint8_t _reserved;
    uint16_t hciversion;
    uint32_t hcsparams1;
    uint32_t hcsparams2;
    uint32_t hcsparams3;
    uint32_t hccparams1;
    uint32_t dboff;
    uint32_t rtsoff;
    uint32_t hccparams2;
}__attribute__((packed));

struct xhci_extended_capabilities {
    uint8_t  cap_id;
    uint8_t  next;
    uint16_t cap_specific;
}__attribute__((packed));

struct xhci_legacy_capability {
    uint8_t cap_id;
    uint8_t next;

    uint8_t hc_bios_owned: 1;
    uint8_t: 7;
    uint8_t hc_os_owned: 1;
    uint8_t: 7;

    uint8_t smi_enable: 1;
    uint8_t: 3;
    uint8_t smi_on_host_system_error_enable: 1;
    uint8_t: 8;
    uint8_t smi_on_os_ownership_enable: 1;
    uint8_t smi_on_pci_command_enable: 1;
    uint8_t smi_on_bar_enable: 1;
    uint8_t smi_on_event_interrupt: 1;
    uint8_t: 3;
    uint8_t smi_on_host_system_error: 1;
    uint8_t: 8;
    uint8_t smi_on_os_ownership: 1;
    uint8_t smi_on_pci_command: 1;
    uint8_t smi_on_bar: 1;
}__attribute__((packed));

struct xhci_dbc_dcid {
    uint8_t cap_id;
    uint8_t next;

    uint16_t dcerst_max: 4;
    uint16_t: 12;
}__attribute__((packed));

struct xhci_dbc_dcdb {
    uint8_t  _reserved1;
    uint8_t  db_target;
    uint16_t _reserved2;
}__attribute__((packed));

struct xhci_dbc_dcerstsz {
    uint16_t erst_size;
    uint16_t _reserved;
}__attribute__((packed));

struct xhci_dbc_dcerstba {
    uint64_t: 4;
    uint64_t dcerstba: 60;
}__attribute__((packed));

struct xhci_dbc_dcerdp {
    uint64_t desi: 3;
    uint64_t: 1;
    uint64_t dequeue_ptr: 60;
}__attribute__((packed));

struct xhci_dbc_dcctrl {
    uint8_t dcr: 1;
    uint8_t lse: 1;
    uint8_t hot: 1;
    uint8_t hit: 1;
    uint8_t drc: 1;
    uint16_t: 11;
    uint8_t debug_max_burst_size;
    uint8_t device_address: 7;
    uint8_t dce: 1;
}__attribute__((packed));

struct xhci_dbc_dcst {
    uint8_t er_not_empty: 1;
    uint8_t dbc_system_bus_reset: 1;
    uint32_t: 22;
    uint8_t debug_port_number;
}__attribute__((packed));

struct xhci_dbc_dcportsc {
    uint8_t current_connect_status: 1;
    uint8_t port_enabled: 1;
    uint8_t: 2;
    uint8_t port_reset: 1;
    uint8_t port_link_state: 4;
    uint8_t: 1;
    uint8_t port_speed: 4;
    uint8_t: 3;
    uint8_t connect_status_change: 1;
    uint8_t: 3;
    uint8_t port_reset_change: 1;
    uint8_t port_link_status_change: 1;
    uint8_t port_config_error_change: 1;
    uint8_t: 8;
}__attribute__((packed));

struct xhci_dbc_dccp {
    uint64_t: 4;
    uint64_t dbc_ctx_ptr: 60;
}__attribute__((packed));

struct xhci_dbc_dccd {
    uint8_t dbc_protocol;
    uint8_t: 8;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t revision;
}__attribute__((packed));

struct xhci_debug_capability {
    struct xhci_dbc_dcid     dcid;
    struct xhci_dbc_dcdb     dcdb;
    struct xhci_dbc_dcerstsz dcerstsz;
    struct xhci_dbc_dcerstba dcerstba;
    struct xhci_dbc_dcerdp   dcerdp;
    struct xhci_dbc_dcctrl   dcctrl;
    struct xhci_dbc_dcst     dcst;
    struct xhci_dbc_dcportsc dcportsc;
    struct xhci_dbc_dccp     dccp;
    struct xhci_dbc_dccd     dccd;
}__attribute__((packed));

struct xhci_event_ring_segment_table_entry {
    uint64_t: 4;
    uint64_t ring_segment: 60;
    uint16_t ring_size;
    uint16_t: 16;
}__attribute__((packed));

union xhci_trb {
    struct {
        uint8_t type_specific1[12];

        uint8_t cycle_bit: 1;
        uint8_t: 1;
        uint8_t event_data: 1;
        uint8_t: 7;
        uint8_t trb_type: 6;

        uint16_t type_specific2;
    }__attribute__((packed)) generic;

    struct {
        uint64_t trb_pointer;
        uint32_t trb_length: 24;
        uint8_t completion_code;

        uint16_t _generic;

        uint8_t endpoint_id: 4;
        uint8_t: 4;
        uint8_t slot_id;
    }__attribute__((packed)) transfer_event;

    struct {
        uint64_t cmd_pointer;
        uint32_t completion_parameter: 24;
        uint8_t completion_code;

        uint16_t _generic;

        uint8_t vf_id;
        uint8_t slot_id;
    }__attribute__((packed)) command_completion;

    struct {
        uint32_t: 24;
        uint8_t port_id;
        uint32_t: 32;
        uint32_t: 24;
        uint8_t completion_code;

        uint16_t _generic;

        uint16_t: 16;
    }__attribute__((packed)) port_status_change;

    struct {
        uint64_t: 64;
        uint32_t: 24;
        uint8_t   completion_code;

        uint16_t _generic;

        uint8_t: 8;
        uint8_t slot_id;
    }__attribute__((packed)) bandwidth_request;

    struct {
        uint8_t db_reason: 5;
        uint64_t: 59;
        uint32_t: 24;
        uint8_t completion_code;

        uint16_t _generic;

        uint8_t vf_id;
        uint8_t slot_id;
    }__attribute__((packed)) doorbell;

    struct {
        uint64_t: 64;
        uint32_t: 24;
        uint8_t   completion_code;

        uint16_t _generic;

        uint16_t: 16;
    }__attribute__((packed)) host_controller;

    struct {
        uint8_t:  4;
        uint8_t   notification_type: 4;
        uint64_t  device_notification_data: 56;
        uint32_t: 24;
        uint8_t   completion_code;

        uint16_t _generic;

        uint8_t: 8;
        uint8_t slot_id;
    }__attribute__((packed)) device_notification;

    struct {
        uint64_t: 64;
        uint32_t: 24;
        uint8_t   completion_code;

        uint16_t _generic;

        uint16_t: 16;
    }__attribute__((packed)) mfindex_wrap;

    struct {
        uint64_t data_ptr;

        uint32_t transfer_length: 17;
        uint8_t  td_size: 5;
        uint16_t interruptor_target: 10;

        uint8_t cycle: 1;
        uint8_t ent: 1;
        uint8_t isp: 1;
        uint8_t no_snoop: 1;
        uint8_t chain: 1;
        uint8_t ioc: 1;
        uint8_t idt: 1;
        uint8_t: 2;
        uint8_t bei: 1;
        uint8_t trb_type: 6;
        uint16_t: 16;
    }__attribute__((packed)) normal;

    struct {
        uint8_t: 4;
        uint64_t segment_pointer: 60;

        uint32_t: 22;
        uint16_t interruptor_target: 10;

        uint8_t cycle: 1;
        uint8_t toggle_cycle: 1;
        uint8_t: 2;
        uint8_t chain: 1;
        uint8_t ioc: 1;
        uint8_t: 4;
        uint8_t trb_type: 6;
        uint16_t: 16;
    }__attribute__((packed)) link;
};

struct dbc_info_context_data {
    uint64_t: 1;
    uint64_t language_string_ptr: 63;

    uint64_t: 1;
    uint64_t manufacturer_string_ptr: 63;

    uint64_t: 1;
    uint64_t product_string_ptr: 63;

    uint64_t: 1;
    uint64_t serial_string_ptr: 63;

    uint8_t language_string_length;
    uint8_t manufacturer_string_length;
    uint8_t product_string_length;
    uint8_t serial_string_length;

    uint64_t: 64;
    uint32_t: 32;
}__attribute__((packed));

struct xhci_endpoint_context {
    uint8_t ep_state: 3;
    uint8_t: 5;
    uint8_t mult: 2;
    uint8_t max_pstreams: 5;
    uint8_t lsa: 1;
    uint8_t interval;
    uint8_t max_esit_payload_hi;

    uint8_t: 1;
    uint8_t cerr: 2;
    uint8_t ep_type: 3;
    uint8_t: 1;
    uint8_t hid: 1;
    uint8_t max_burst_size;
    uint16_t max_packet_size;

    uint8_t dcs: 1;
    uint8_t: 3;
    uint64_t tr_dequeue_pointer: 60;

    uint16_t average_trb_length;
    uint16_t max_esit_payload_lo;

    uint64_t: 64;
    uint32_t: 32;
}__attribute__((packed));

struct dbc_endpoint_context {
    struct xhci_endpoint_context xhci;

    uint64_t: 64;
    uint64_t: 64;
    uint64_t: 64;
    uint64_t: 64;
}__attribute__((packed));

struct dbc_context {
    struct dbc_info_context_data info_data;
    struct dbc_endpoint_context  out_endpoint_context;
    struct dbc_endpoint_context  in_endpoint_context;
}__attribute__((packed));

#endif
