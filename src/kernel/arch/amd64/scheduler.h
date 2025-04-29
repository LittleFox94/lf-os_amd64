#ifndef _SCHEDULER_H_INCLUDED
#define _SCHEDULER_H_INCLUDED

#include <stdint.h>
#include <vm.h>
#include <cpu.h>
#include <memory/context.h>

#include <memory>

#if !defined(LF_OS_TESTING)
typedef uint64_t pid_t;
#endif

enum kill_reason {
    kill_reason_segv  = 1,
    kill_reason_abort = 2,
};

enum wait_reason {
    wait_reason_mutex,
    wait_reason_condvar,
    wait_reason_message,
    wait_reason_time,
};

extern pid_t scheduler_current_process;

#include <mutex.h>
#include <condvar.h>

union wait_data {
    mutex_t   mutex;
    condvar_t condvar;
    uint64_t  message_queue;
    uint64_t  timestamp_ns_since_boot;
};

void init_scheduler(void);
void start_task(std::shared_ptr<MemoryContext> context, uint64_t entry, const char* name);

void schedule_next(cpu_state** cpu, struct vm_table** context);
bool schedule_next_if_needed(cpu_state** cpu, struct vm_table** context);
void scheduler_process_save(cpu_state* cpu);

bool scheduler_handle_pf(uint64_t fault_address, uint64_t error_code);
void scheduler_kill_current(enum kill_reason kill_reason);

void scheduler_wait_for(pid_t pid, enum wait_reason reason, union wait_data data);
void scheduler_waitable_done(enum wait_reason reason, union wait_data data, size_t max_amount);

//! Map a given memory area in the currently running userspace process at a random location
uint64_t scheduler_map_hardware(uint64_t hw, size_t len);

#endif
