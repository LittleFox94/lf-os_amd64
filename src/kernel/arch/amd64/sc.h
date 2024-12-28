#ifndef _SC_H_INCLUDED
#define _SC_H_INCLUDED

#include <stdint.h>
#include <vm.h>

void init_gdt(void);
void init_sc(void);

void interrupt_add_queue(uint8_t interrupt, uint64_t mq);
void interrupt_del_queue(uint8_t interrupt, uint64_t mq);

void set_iopb(struct vm_table* context, uint64_t task_iopb);

#endif
