#ifndef _SCHEDULER_H_INCLUDED
#define _SCHEDULER_H_INCLUDED

#include "vm.h"
#include "cpu.h"

enum kill_reason_t {
    kill_reason_segv  = 1,
    kill_reason_abort = 2,
};

void init_scheduler();
void start_task(vm_table_t* context, ptr_t entry, ptr_t data_start, ptr_t data_end);

void schedule_next(cpu_state** cpu, vm_table_t** context);
void schedule_available(cpu_state** cpu, vm_table_t** context);
void scheduler_process_save(cpu_state* cpu);

bool scheduler_handle_pf(ptr_t fault_address);
void scheduler_kill_current(enum kill_reason_t kill_reason);

#endif
