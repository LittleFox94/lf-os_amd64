#include <scheduler.h>
#include <string.h>
#include <mm.h>
#include <bluescreen.h>
#include <log.h>
#include <mutex.h>

typedef enum {
    process_state_empty = 0,
    process_state_waiting,
    process_state_runnable,
    process_state_running,
    process_state_exited,
    process_state_killed,
} process_state;

typedef struct {
    struct vm_table* context;
    cpu_state        cpu;
    process_state    state;
    region_t         heap;
    region_t         stack;
    uint8_t          exit_code;

    enum wait_reason waiting_for;
    union wait_data  waiting_data;
} process_t;

volatile pid_t scheduler_current_process = -1;

// TODO: better data structure here. A binary tree with prev/next pointers should be great
#define MAX_PROCS 4096
process_t processes[MAX_PROCS];

void init_scheduler() {
    memset((uint8_t*)processes, 0, sizeof(process_t) * MAX_PROCS);
}

pid_t free_pid() {
    pid_t i;
    for(i = 0; i < MAX_PROCS; ++i) {
        if(processes[i].state == process_state_empty) {
            return i;
        }
    }

    return -1;
}

pid_t setup_process() {
    pid_t pid = free_pid();
    if(pid == -1) {
        panic_message("Out of PIDs!");
    }

    process_t* process = &processes[pid];
    memset((void*)&process->cpu, 0, sizeof(cpu_state));

    process->state       = process_state_runnable;
    process->cpu.cs      = 0x2B;
    process->cpu.ss      = 0x23;
    process->cpu.rflags  = 0x200;

    process->heap.start = 0;
    process->heap.end   = 0;

    process->stack.start = ALLOCATOR_REGION_USER_STACK.end;
    process->stack.end   = ALLOCATOR_REGION_USER_STACK.end;

    return pid;
}

void start_task(struct vm_table* context, ptr_t entry, ptr_t data_start, ptr_t data_end) {
    if(!entry) {
        panic_message("Tried to start process without entry");
    }

    pid_t pid            = setup_process();
    process_t* process = &processes[pid];

    process->context = context;
    process->cpu.rip = entry;
    process->cpu.rsp = ALLOCATOR_REGION_USER_STACK.end;

    process->heap.start = data_start;
    process->heap.end   = data_end;
}

void scheduler_process_save(cpu_state* cpu) {
    if(scheduler_current_process >= 0 &&
        (processes[scheduler_current_process].state == process_state_running ||
         processes[scheduler_current_process].state == process_state_waiting)
    ) {
        memcpy(&processes[scheduler_current_process].cpu, cpu, sizeof(cpu_state));
    }
}

void schedule_next(cpu_state** cpu, struct vm_table** context) {
    if(scheduler_current_process >= 0 && processes[scheduler_current_process].state == process_state_running) {
        processes[scheduler_current_process].state = process_state_runnable;
    }

    for(int i = 1; i <= MAX_PROCS; ++i) {
        pid_t pid = (scheduler_current_process + i) % MAX_PROCS;

        if(processes[pid].state == process_state_runnable) {
            scheduler_current_process = pid;
            break;
        }
    }

    if(processes[scheduler_current_process].state != process_state_runnable) {
        panic_message("No more tasks to schedule");
    }

    processes[scheduler_current_process].state = process_state_running;
    *cpu     = &processes[scheduler_current_process].cpu;
    *context =  processes[scheduler_current_process].context;
}

void scheduler_process_cleanup(pid_t pid) {
    mutex_unlock_holder(pid);
}

void scheduler_kill_current(enum kill_reason reason) {
    processes[scheduler_current_process].state     = process_state_killed;
    processes[scheduler_current_process].exit_code = (int)reason;
    logd("scheduler", "killed PID %d (reason: %d)", scheduler_current_process, (int)reason);

    scheduler_process_cleanup(scheduler_current_process);
}

void sc_handle_scheduler_exit(uint64_t exit_code) {
    processes[scheduler_current_process].state     = process_state_exited;
    processes[scheduler_current_process].exit_code = exit_code;
    logd("scheduler", "PID %d exited (status: %d)", scheduler_current_process, exit_code);

    scheduler_process_cleanup(scheduler_current_process);
}

void sc_handle_scheduler_clone(bool share_memory, ptr_t entry, pid_t* newPid) {
    // make new process
    pid_t pid = setup_process();

    if(pid < 0) {
        *newPid = -1;
        return;
    }

    process_t* process = &processes[pid];

    // new memory context ...
    struct vm_table* context = vm_context_new();
    process->context         = context;

    // .. copy heap ..
    if(!share_memory) {
        for(size_t i = processes[scheduler_current_process].heap.start; i <= processes[scheduler_current_process].heap.end; i += 0x1000) {
            vm_copy_page(context, (ptr_t)i, processes[scheduler_current_process].context, (ptr_t)i);
        }
    }
    else {
        // make heap shared
    }

    process->heap.start = processes[scheduler_current_process].heap.start;
    process->heap.end   = processes[scheduler_current_process].heap.end;

    // .. and stack ..
    for(size_t i = processes[scheduler_current_process].stack.start; i < processes[scheduler_current_process].stack.end; i += 0x1000) {
        vm_copy_page(context, (ptr_t)i, processes[scheduler_current_process].context, (ptr_t)i);
    }

    process->stack.start = processes[scheduler_current_process].stack.start;
    process->stack.end   = processes[scheduler_current_process].stack.end;

    // .. and remap hardware resources
    for(ptr_t i = ALLOCATOR_REGION_USER_HARDWARE.start; i < ALLOCATOR_REGION_USER_HARDWARE.end; i += 4096) {
        ptr_t hw = vm_context_get_physical_for_virtual(processes[scheduler_current_process].context, i);

        if(hw) {
            vm_context_map(context, i, hw);
        }
    }

    if(share_memory && entry) {
        process->cpu.rip = entry;
    }

    *newPid = pid;

    // copy cpu state
    memcpy(&process->cpu, &processes[scheduler_current_process].cpu, sizeof(cpu_state));
    process->cpu.rax = 0;
}

bool scheduler_handle_pf(ptr_t fault_address, uint64_t error_code) {
    if(fault_address >= ALLOCATOR_REGION_USER_STACK.start && fault_address < ALLOCATOR_REGION_USER_STACK.end) {
        ptr_t page_v = fault_address & ~0xFFF;
        ptr_t page_p = (ptr_t)mm_alloc_pages(1);
        memset((void*)(page_p + ALLOCATOR_REGION_DIRECT_MAPPING.start), 0, 0x1000);
        vm_context_map(processes[scheduler_current_process].context, page_v, page_p);

        if(page_v < processes[scheduler_current_process].stack.start) {
            processes[scheduler_current_process].stack.start = page_v;
        }

        return true;
    }

    logw("scheduler", "Not handling page fault for %d at 0x%x (RIP: 0x%x, error 0x%x)", scheduler_current_process, fault_address, processes[scheduler_current_process].cpu.rip, error_code);

    return false;
}

void scheduler_wait_for(pid_t pid, enum wait_reason reason, union wait_data data) {
    if(pid == -1) {
        pid = scheduler_current_process;
    }

    processes[pid].state        = process_state_waiting;
    processes[pid].waiting_for  = reason;
    processes[pid].waiting_data = data;
}

void scheduler_waitable_done(enum wait_reason reason, union wait_data data, size_t max_amount) {
    for(int i = 0; i < MAX_PROCS; ++i) {
        pid_t pid = (scheduler_current_process + i) % MAX_PROCS;

        process_t* p = &processes[pid];

        if(p->state == process_state_waiting && p->waiting_for == reason) {
            switch(reason) {
                case wait_reason_mutex:
                    if(p->waiting_data.mutex == data.mutex) {
                        if(mutex_lock(data.mutex, pid)) {
                            p->state = process_state_runnable;
                        }
                    }
                    break;
                case wait_reason_condvar:
                    if(p->waiting_data.condvar == data.condvar && max_amount--) {
                        p->state = process_state_runnable;
                    }
                    break;
            }
        }
    }
}

void sc_handle_memory_sbrk(int64_t inc, ptr_t* data_end) {
    ptr_t old_end = processes[scheduler_current_process].heap.end;
    ptr_t new_end = old_end + inc;

    if(inc > 0) {
        for(ptr_t i = old_end & ~0xFFF; i < new_end; i+=0x1000) {
            if(!vm_context_get_physical_for_virtual(processes[scheduler_current_process].context, i)) {
                ptr_t phys = (ptr_t)mm_alloc_pages(1);
                memset((void*)(phys + ALLOCATOR_REGION_DIRECT_MAPPING.start), 0, 0x1000);
                vm_context_map(processes[scheduler_current_process].context, i, phys);
            }
        }
    }
    if(inc < 0) {
        for(ptr_t i = old_end; i > new_end; i -= 0x1000) {
            if(!(i & ~0xFFF)) {
                mm_mark_physical_pages(vm_context_get_physical_for_virtual(processes[scheduler_current_process].context, i), 1, MM_FREE);
                // TODO: unmap
            }
        }
    }

    processes[scheduler_current_process].heap.end = new_end;
    *data_end = new_end;
}

void sc_handle_scheduler_yield() {
    // no-op
}
