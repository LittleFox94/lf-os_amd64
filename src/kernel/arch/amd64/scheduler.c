#include "scheduler.h"
#include "string.h"
#include "mm.h"
#include "bluescreen.h"

typedef enum {
    process_state_empty = 0,
    process_state_runnable,
    process_state_running,
    process_state_exited,
    process_state_killed,
} process_state;

typedef struct {
    vm_table_t*     context;
    cpu_state       cpu;
    process_state   state;
    region_t        heap;
    region_t        stack;
    uint8_t         exit_code;
} process_t;

volatile int scheduler_current_process = -1;

// TODO: better data structure here. A binary tree with prev/next pointers should be great
#define MAX_PROCS 4096
process_t processes[MAX_PROCS];

void init_scheduler() {
    memset((uint8_t*)processes, 0, sizeof(process_t) * MAX_PROCS);
}

int free_pid() {
    int i;
    for(i = 0; i < MAX_PROCS; ++i) {
        if(processes[i].state == process_state_empty) {
            return i;
        }
    }

    return -1;
}

int setup_process() {
    int pid = free_pid();
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

void start_task(vm_table_t* context, ptr_t entry, ptr_t data_start, ptr_t data_end) {
    if(!entry) {
        panic_message("Tried to start process without entry");
    }

    int pid            = setup_process();
    process_t* process = &processes[pid];

    process->context = context;
    process->cpu.rip = entry;
    process->cpu.rsp = ALLOCATOR_REGION_USER_STACK.end;

    process->heap.start = data_start;
    process->heap.end   = data_end;
}

void schedule_next(cpu_state** cpu, vm_table_t** context) {
    if(scheduler_current_process > -1 && processes[scheduler_current_process].state == process_state_running) {
        processes[scheduler_current_process].state = process_state_runnable;
        memcpy(&processes[scheduler_current_process].cpu, *cpu, sizeof(cpu_state));
    }

    int processBefore = scheduler_current_process;
    for(int i = processBefore + 1; i < MAX_PROCS; ++i) {
        if(processes[i].state == process_state_runnable) {
            scheduler_current_process = i;
            break;
        }
    }

    if(scheduler_current_process == processBefore) {
        for(int i = 0; i <= processBefore; ++i) {
            if(processes[i].state == process_state_runnable) {
                scheduler_current_process = i;
                break;
            }
        }
    }

    if(processes[scheduler_current_process].state != process_state_runnable) {
        panic_message("No more tasks to schedule");
    }

    processes[scheduler_current_process].state = process_state_running;
    *cpu     = &processes[scheduler_current_process].cpu;
    *context =  processes[scheduler_current_process].context;
}

void schedule_available(cpu_state** cpu, vm_table_t** context) {
    if(scheduler_current_process < 0 || processes[scheduler_current_process].state != process_state_running) {
        schedule_next(cpu, context);
        return;
    }

    *context = processes[scheduler_current_process].context;
}

void scheduler_process_save(cpu_state* cpu) {
    if(scheduler_current_process >= 0) {
        memcpy(&processes[scheduler_current_process].cpu, cpu, sizeof(cpu_state));
    }
}

void scheduler_kill_current(enum kill_reason_t reason) {
    processes[scheduler_current_process].state     = process_state_killed;
    processes[scheduler_current_process].exit_code = (int)reason;
    fbconsole_write("[PID %04d] killed: %d\n", scheduler_current_process, (int)reason);
}

void sc_handle_scheduler_exit(uint64_t exit_code) {
    processes[scheduler_current_process].state     = process_state_exited;
    processes[scheduler_current_process].exit_code = exit_code;
    fbconsole_write("[PID %04d] exited: %d\n", scheduler_current_process, exit_code);
}

void sc_handle_scheduler_clone(bool share_memory, ptr_t entry, uint64_t* newPid) {
    // make new process
    uint64_t pid = setup_process();
    process_t* process = &processes[pid];

    // new memory context ...
    vm_table_t* context = vm_context_new();
    process->context    = context;

    // .. copy heap ..
    if(!share_memory) {
        for(size_t i = processes[scheduler_current_process].heap.start; i < processes[scheduler_current_process].heap.end; i += 0x1000) {
            vm_copy_page(context, (ptr_t)i, processes[scheduler_current_process].context, (ptr_t)i);
        }
    }
    else {
        // make heap shared
    }

    process->heap.start = processes[scheduler_current_process].heap.start;
    process->heap.end   = processes[scheduler_current_process].heap.end;

    // .. and stack
    for(size_t i = processes[scheduler_current_process].stack.end; i >= processes[scheduler_current_process].stack.start; i -= 0x1000) {
        vm_copy_page(context, (ptr_t)i, processes[scheduler_current_process].context, (ptr_t)i);
    }

    process->stack.start = processes[scheduler_current_process].stack.start;
    process->stack.end   = processes[scheduler_current_process].stack.end;

    if(share_memory && entry) {
        process->cpu.rip = entry;
    }

    *newPid = pid;

    // copy cpu state, set return value
    memcpy(&process->cpu, &processes[scheduler_current_process].cpu, sizeof(cpu_state));
    process->cpu.rax = 0;
}

bool scheduler_handle_pf(ptr_t fault_address) {
    if(fault_address >= ALLOCATOR_REGION_USER_STACK.start && fault_address < ALLOCATOR_REGION_USER_STACK.end) {
        ptr_t page_v = fault_address & 0xFFFFFFFFFFFFF000;
        ptr_t page_p = (ptr_t)mm_alloc_pages(1);
        vm_context_map(processes[scheduler_current_process].context, page_v, page_p);

        if(page_v < processes[scheduler_current_process].stack.start) {
            processes[scheduler_current_process].stack.start = page_v;
        }

        return true;
    }

    fbconsole_write("[PID %04d] Not handling page fault at 0x%x\n", scheduler_current_process, fault_address);

    return false;
}

void sc_handle_memory_sbrk(uint64_t inc, ptr_t* data_end) {
    if(inc) {
        inc += 0x1000 - (inc % 0x1000);

        ptr_t old_end = (processes[scheduler_current_process].heap.end + 1) & 0xFFFFFFFFFFFFF000;
        ptr_t new_end = old_end + inc;

        for(ptr_t i = old_end; i < new_end; i+=0x1000) {
            vm_context_map(processes[scheduler_current_process].context, i, (ptr_t)mm_alloc_pages(1));
        }

        processes[scheduler_current_process].heap.end = new_end;
    }

    *data_end = processes[scheduler_current_process].heap.end;
}
