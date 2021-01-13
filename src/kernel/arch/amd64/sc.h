#ifndef _SC_H_INCLUDED
#define _SC_H_INCLUDED

#include <stdint.h>
#include <vm.h>

void init_gdt();
void init_sc();

void set_iopb(struct vm_table* context, ptr_t task_iopb);

#endif
