#ifndef _VM_H_INCLUDED
#define _VM_H_INCLUDED

#include "stdint.h"
#include "stdbool.h"

typedef struct {
    char* name;
    ptr_t start;
    ptr_t end;
} region_t;

#define ALLOCATOR_REGION_NULL           (region_t){ .name = "NULL", .start = 0, .end = 0 }

// Stack has to be 16-bytes aligned
#define ALLOCATOR_REGION_USER_STACK     (region_t){ .name = "User stack", .start = 0x00007F0000001000, .end = 0x00007FFFFFFFEFF0 }

#define ALLOCATOR_REGION_SCRATCHPAD     (region_t){ .name = "Kernel scratchpad", .start = 0xFFFF800000000000, .end = 0xFFFF800000FFFFFF }
#define ALLOCATOR_REGION_KERNEL_BINARY  (region_t){ .name = "Kernel binary",     .start = 0xFFFF800001000000, .end = 0xFFFF800008FFFFFF }
#define ALLOCATOR_REGION_SLAB_4K        (region_t){ .name = "4k slab allocator", .start = 0xFFFF800009000000, .end = 0xFFFF80000FFFFFFF }
#define ALLOCATOR_REGION_KERNEL_HEAP    (region_t){ .name = "Kernel heap",       .start = 0xFFFF800040000000, .end = 0xFFFF8007FFFFFFFF }

// this one must be PML4 aligned! (PDP, PD and PT indexes must be zero for the start and 511 for the end)
#define ALLOCATOR_REGION_DIRECT_MAPPING (region_t){ .name = "Physical mapping",  .start = 0xFFFF840000000000, .end = 0xFFFF87FFFFFFFFFF }

//! A single entry in a paging table
struct vm_table_entry {
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
}__attribute__((packed));

//! A paging table, when this is a PML4 it may also be called context
struct vm_table {
    struct vm_table_entry entries[512];
}__attribute__((packed));

struct vm_table* VM_KERNEL_CONTEXT;

void init_vm();
void cleanup_boot_vm();

struct vm_table* vm_context_new();

void vm_context_activate(struct vm_table* context);

void vm_context_map(struct vm_table* context, ptr_t virtual, ptr_t physical);

ptr_t vm_context_get_free_address(struct vm_table* table, bool kernel);

int   vm_table_get_free_index1(struct vm_table* table);
int   vm_table_get_free_index3(struct vm_table* table, int start, int end);

ptr_t vm_context_get_physical_for_virtual(struct vm_table* context, ptr_t virtual);

ptr_t vm_context_alloc_pages(struct vm_table* context, region_t region, size_t num);

void vm_copy_page(struct vm_table* dst_ctx, ptr_t dst, struct vm_table* src_ctx, ptr_t src);

#endif
