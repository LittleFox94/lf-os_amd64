#ifndef _SCHEDULER_H_INCLUDED
#define _SCHEDULER_H_INCLUDED

#include "vm.h"
#include "cpu.h"

void init_scheduler();
void start_task(vm_table_t* context, ptr_t entry);

void schedule_next(cpu_state** cpu, vm_table_t** context);

#endif
