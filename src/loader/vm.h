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
    unsigned int present      : 1;
    unsigned int writeable    : 1;
    unsigned int userspace    : 1;
    unsigned int writethrough : 1;
    unsigned int cachedisable : 1;
    unsigned int accessed     : 1;
    unsigned int dirty        : 1;
    unsigned int huge         : 1;
    unsigned int global       : 1;
    unsigned int available    : 3;
    unsigned long next_base   : 40;
    unsigned int available2   : 11;
    unsigned int nx           : 1;
}__attribute__((packed)) vm_table_entry_t;

typedef struct {
    vm_table_entry_t entries[512];
}__attribute__((packed)) vm_table_t;

#endif /* VM_H_INCLUDED */
