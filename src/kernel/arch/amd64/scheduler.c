#include "scheduler.h"
#include "string.h"
#include "mm.h"
#include "bluescreen.h"

typedef struct {
    vm_table_t* context;
    cpu_state   cpu;
    bool        schedulable;
} process_t;

volatile int scheduler_current_process = -1;

// TODO: better data structure here. A binary tree with prev/next pointers should be great
#define MAX_PROCS 256
process_t processes[MAX_PROCS];

void init_scheduler() {
    memset((uint8_t*)processes, 0, sizeof(process_t) * MAX_PROCS);
}

int free_pid() {
    int i;
    for(i = 0; i < MAX_PROCS; ++i) {
        if(!processes[i].schedulable) {
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

    process->schedulable = true;
    process->cpu.cs      = 0x2B;
    process->cpu.ss      = 0x23;
    process->cpu.rflags  = 0x200;


    return pid;
}

void start_task(vm_table_t* context, ptr_t entry, ptr_t stack) {
    if(!entry) {
        panic_message("Tried to start process without entry");
    }

    int pid = setup_process();
    process_t* process = &processes[pid];

    process->context     = context;
    process->cpu.rip     = entry;
    process->cpu.rsp     = stack;

    for(ptr_t map_for_stack = processes[pid].cpu.rsp; map_for_stack >= processes[pid].cpu.rsp - (1*MiB); map_for_stack -= 0x1000) {
        vm_context_map(context, map_for_stack, (ptr_t)mm_alloc_pages(1));
    }
}

void schedule_next(cpu_state** cpu, vm_table_t** context) {
    if(scheduler_current_process > -1) {
        memcpy(&processes[scheduler_current_process].cpu, *cpu, sizeof(cpu_state));
    }
    else {
        scheduler_current_process = 0;
    }

    int processBefore = scheduler_current_process;
    for(int i = processBefore + 1; i < MAX_PROCS; ++i) {
        if(processes[i].schedulable) {
            scheduler_current_process = i;
            break;
        }
    }

    if(scheduler_current_process == processBefore) {
        for(int i = 0; i <= processBefore; ++i) {
            if(processes[i].schedulable) {
                scheduler_current_process = i;
                break;
            }
        }
    }

    if(scheduler_current_process == processBefore && !processes[processBefore].schedulable) {
        panic_message("No more tasks to schedule");
    }

    *cpu     = &processes[scheduler_current_process].cpu;
    *context =  processes[scheduler_current_process].context;
}

void sc_handle_scheduler_exit(uint64_t pid) {
    nyi(1);
}

void sc_handle_scheduler_clone(bool share_memory, ptr_t child_stack, ptr_t entry, uint64_t* newPid) {
    int pid = setup_process();
    process_t* process = &processes[pid];

    memcpy(process, &processes[scheduler_current_process], sizeof(process_t));
    process->cpu.rax = 0;

    if(share_memory) {
        process->cpu.rsp = child_stack;

        if(entry) {
            process->cpu.rip = entry;
        }
    }
    else {
        // free heap
        // how to start an image then?
        nyi(1);
    }

    *newPid = pid;
}
