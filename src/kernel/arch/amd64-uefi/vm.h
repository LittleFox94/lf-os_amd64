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
 * Copyright (c) 2018 M "LittleFox" Grosch <littlefox@lf-net.org>
 *   -- 2018-08-08
 */

#include "stdint.h"
#include "stdbool.h"

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

#endif
