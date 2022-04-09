#ifndef _SCHEDULER_H_INCLUDED
#define _SCHEDULER_H_INCLUDED

#include <stdint.h>
#include <vm.h>
#include <cpu.h>

enum kill_reason {
    kill_reason_segv  = 1,
    kill_reason_abort = 2,
};

enum wait_reason {
    wait_reason_mutex,
    wait_reason_condvar,
    wait_reason_message,
};

extern volatile pid_t scheduler_current_process;

#include <mutex.h>
#include <condvar.h>

union wait_data {
    mutex_t   mutex;
    condvar_t condvar;
    uint64_t  message_queue;
};

void init_scheduler();
void start_task(struct vm_table* context, ptr_t entry, ptr_t data_start, ptr_t data_end, const char* name);

void schedule_next(cpu_state** cpu, struct vm_table** context);
void scheduler_process_save(cpu_state* cpu);
bool scheduler_idle_if_needed(cpu_state** cpu, struct vm_table** context);

bool scheduler_handle_pf(ptr_t fault_address, uint64_t error_code);
void scheduler_kill_current(enum kill_reason kill_reason);

void scheduler_wait_for(pid_t pid, enum wait_reason reason, union wait_data data);
void scheduler_waitable_done(enum wait_reason reason, union wait_data data, size_t max_amount);

//! Map a given memory area in the currently running userspace process at a random location
ptr_t scheduler_map_hardware(ptr_t hw, size_t len);

#endif
