#include "scheduler.h"
#include "string.h"
#include "mm.h"

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

void start_task(vm_table_t* context, ptr_t entry) {
    if(!entry) {
        while(1);
    }

    int i;

    for(i = 0; i < MAX_PROCS; ++i) {
        if(!processes[i].schedulable) {
            break;
        }
    }

    processes[i].schedulable = true;
    processes[i].context     = context;
    processes[i].cpu.rip     = entry;
    processes[i].cpu.cs      = 0x23;
    processes[i].cpu.ss      = 0x28;
    processes[i].cpu.rsp     = (ptr_t)1024 * 1024 * 4 * 1024;

    for(ptr_t map_for_stack = processes[i].cpu.rsp; map_for_stack >= 0x1000; map_for_stack -= 0x1000) {
        vm_context_map(context, map_for_stack, (ptr_t)mm_alloc_kernel_pages(1));
    }

    if(scheduler_current_process == -1) {
        scheduler_current_process = i;
    }
}

void schedule_next(cpu_state** cpu, vm_table_t** context) {
    memcpy(&processes[scheduler_current_process].cpu, *cpu, sizeof(cpu_state));

    *cpu     = &processes[scheduler_current_process].cpu;
    *context =  processes[scheduler_current_process].context;
    DUMP_CPU((*cpu));
}
