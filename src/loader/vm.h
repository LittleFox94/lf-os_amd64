#ifndef _VM_H_INCLUDED
#define _VM_H_INCLUDED

#define PML4_INDEX(x) (x >> 39) & 0x1FF
#define PDP_INDEX(x)  (x >> 30) & 0x1FF
#define PD_INDEX(x)   (x >> 21) & 0x1FF
#define PT_INDEX(x)   (x >> 12) & 0x1FF

#define PAGING_ENTRY_DEFAULT_SETTINGS   \
    .present        = 1,                \
    .writeable      = 1,                \
    .userspace      = 1,                \
    .writethrough   = 0,                \
    .cachedisable   = 0,                \
    .accessed       = 0,                \
    .dirty          = 0,                \
    .huge           = 0,                \
    .global         = 0,                \
    .available      = 0,                \
    .available2     = 0,                \
    .nx             = 0

typedef struct {
    unsigned long long present      : 1;
    unsigned long long writeable    : 1;
    unsigned long long userspace    : 1;
    unsigned long long writethrough : 1;
    unsigned long long cachedisable : 1;
    unsigned long long accessed     : 1;
    unsigned long long dirty        : 1;
    unsigned long long huge         : 1;
    unsigned long long global       : 1;
    unsigned long long available    : 3;
    unsigned long long next_base    : 40;
    unsigned long long available2   : 11;
    unsigned long long nx           : 1;
}__attribute__((packed)) vm_table_entry_t;

typedef struct {
    vm_table_entry_t entries[512];
}__attribute__((packed)) vm_table_t;

#endif /* VM_H_INCLUDED */
