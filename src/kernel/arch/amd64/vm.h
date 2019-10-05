#ifndef _VM_H_INCLUDED
#define _VM_H_INCLUDED

/** Virtual memory management module
 *
 * Responsible for allocating free virtual addresses in contexts,
 * creating and destroying contexts and activating them.
 *
 * A context is the thing you put in CR3.
 *
 * LF OS amd64-uefi
 * Copyright (c) 2018 Mara Sophie "LittleFox" Grosch <littlefox@lf-net.org>
 *   -- 2018-08-08
 */

#include "stdint.h"
#include "stdbool.h"

typedef struct {
    char* name;
    ptr_t start;
    ptr_t end;
} region_t;

#define ALLOCATOR_REGION_NULL           (region_t){ .name = "NULL", .start = 0, .end = 0 }

#define ALLOCATOR_REGION_USER_HEAP      (region_t){ .name = "User heap",  .start = 0x0000100000000000, .end = 0x00007F0000000000 }
// Stack has to be 16-bytes aligned
#define ALLOCATOR_REGION_USER_STACK     (region_t){ .name = "User stack", .start = 0x00007F0000001000, .end = 0x00007FFFFFFFFFF0 }

#define ALLOCATOR_REGION_SCRATCHPAD     (region_t){ .name = "Kernel scratchpad", .start = 0xFFFF800000000000, .end = 0xFFFF800000FFFFFF }
#define ALLOCATOR_REGION_KERNEL_BINARY  (region_t){ .name = "Kernel binary",     .start = 0xFFFF800001000000, .end = 0xFFFF800008FFFFFF }
#define ALLOCATOR_REGION_SLAB_4K        (region_t){ .name = "4k slab allocator", .start = 0xFFFF800009000000, .end = 0xFFFF80000FFFFFFF }
#define ALLOCATOR_REGION_KERNEL_HEAP    (region_t){ .name = "Kernel heap",       .start = 0xFFFF800040000000, .end = 0xFFFF8007FFFFFFFF }

// this one must be PML4 aligned! (PDP, PD and PT indexes must be zero for the start and 511 for the end)
#define ALLOCATOR_REGION_DIRECT_MAPPING (region_t){ .name = "Physical mapping",  .start = 0xFFFF840000000000, .end = 0xFFFF87FFFFFFFFFF }

#define ALLOCATOR_REGIONS (region_t[]){ \
    ALLOCATOR_REGION_USER_HEAP,      \
    ALLOCATOR_REGION_USER_STACK,     \
    ALLOCATOR_REGION_SCRATCHPAD,     \
    ALLOCATOR_REGION_KERNEL_BINARY,  \
    ALLOCATOR_REGION_SLAB_4K,        \
    ALLOCATOR_REGION_KERNEL_HEAP,    \
    ALLOCATOR_REGION_DIRECT_MAPPING, \
    ALLOCATOR_REGION_NULL,           \
}

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

vm_table_t* VM_KERNEL_CONTEXT;

void init_vm();

vm_table_t* vm_context_new();

void vm_context_activate(vm_table_t* context);

void vm_context_map(vm_table_t* context, ptr_t virtual, ptr_t physical);

ptr_t vm_context_get_free_address(vm_table_t* table, bool kernel);

bool  vm_table_check_present(vm_table_t* table, int index);

int   vm_table_get_free_index1(vm_table_t* table);
int   vm_table_get_free_index3(vm_table_t* table, int start, int end);

ptr_t vm_context_get_physical_for_virtual(vm_table_t* context, ptr_t virtual);

ptr_t vm_context_alloc_pages(vm_table_t* context, region_t region, size_t num);

void vm_copy_page(vm_table_t* dst_ctx, ptr_t dst, vm_table_t* src_ctx, ptr_t src); 

#endif
