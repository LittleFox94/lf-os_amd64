#ifndef _SC_H_INCLUDED
#define _SC_H_INCLUDED

#include <stdint.h>

void init_gdt();
void init_sc();

void set_iopb(ptr_t task_iopb);

#endif
