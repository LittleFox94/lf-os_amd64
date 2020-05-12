#ifndef _SCHEDULER_H_INCLUDED
#define _SCHEDULER_H_INCLUDED

#include "vm.h"
#include "cpu.h"

enum kill_reason_t {
    kill_reason_segv  = 1,
    kill_reason_abort = 2,
};

void init_scheduler();
void start_task(struct vm_table* context, ptr_t entry, ptr_t data_start, ptr_t data_end);

void schedule_next(cpu_state** cpu, struct vm_table** context);
void scheduler_process_save(cpu_state* cpu);

bool scheduler_handle_pf(ptr_t fault_address, uint64_t error_code);
void scheduler_kill_current(enum kill_reason_t kill_reason);

#endif
