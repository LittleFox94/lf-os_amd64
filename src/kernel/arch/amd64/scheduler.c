#include <scheduler.h>
#include <string.h>
#include <errno.h>
#include <mm.h>
#include <sc.h>
#include <bitmap.h>
#include <panic.h>
#include <log.h>
#include <mutex.h>
#include <mq.h>
#include <signal.h>
#include <sd.h>

typedef enum {
    process_state_empty = 0,
    process_state_waiting,
    process_state_runnable,
    process_state_running,
    process_state_exited,
    process_state_killed,
} process_state;

typedef struct {
    char name[1024];

    pid_t         parent;
    uint8_t       exit_code;
    process_state state;

    enum wait_reason waiting_for;
    union wait_data  waiting_data;

    region_t         heap;
    region_t         stack;
    region_t         hw;
    mq_id_t          mq;
    struct vm_table* context;
    cpu_state        cpu;

    //! Physical address of the first of two IOPB pages, 0 if no IO privilege granted
    ptr_t iopb;

    allocator_t allocator;
    size_t allocatedMemory;
} process_t;

volatile pid_t scheduler_current_process = -1;

// TODO: better data structure here. A binary tree with prev/next pointers should be great
#define MAX_PROCS 4096
static process_t processes[MAX_PROCS];

void* process_alloc(allocator_t* alloc, size_t size) {
    if(!alloc                                               ||
        processes[alloc->tag].state == process_state_exited ||
        processes[alloc->tag].state == process_state_killed ||
        processes[alloc->tag].state == process_state_empty
    ) {
        logw("scheduler", "process_alloc called for invalid process %u\n", alloc->tag);
        return 0;
    }

    processes[alloc->tag].allocatedMemory += size;

    return vm_alloc(size);
}

void process_dealloc(allocator_t* alloc, void* ptr) {
    if(!alloc || processes[alloc->tag].state == process_state_empty) {
        logw("scheduler", "process_dealloc called for invalid process %u\n", alloc->tag);
        return;
    }

    size_t size = *((size_t*)ptr - 1);
    vm_free(ptr);

    processes[alloc->tag].allocatedMemory -= size;
}

__attribute__((naked)) static void idle_task() {
    asm("jmp .");
}

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

    process->hw.start = ALLOCATOR_REGION_USER_HARDWARE.start;
    process->hw.end   = ALLOCATOR_REGION_USER_HARDWARE.start;

    process->allocator = (allocator_t){
        .alloc   = process_alloc,
        .dealloc = process_dealloc,
        .tag     = pid,
    };

    process->mq = mq_create(&process->allocator);

    return pid;
}

void start_task(struct vm_table* context, ptr_t entry, ptr_t data_start, ptr_t data_end, const char* name) {
    if(!entry) {
        panic_message("Tried to start process without entry");
    }

    pid_t pid          = setup_process();
    process_t* process = &processes[pid];

    process->parent  = -1;
    process->context = context;
    process->cpu.rip = entry;
    process->cpu.rsp = ALLOCATOR_REGION_USER_STACK.end;

    process->heap.start = data_start;
    process->heap.end   = data_end;

    strncpy(process->name, name, 1023);
}

void scheduler_process_save(cpu_state* cpu) {
    if(scheduler_current_process >= 0 &&
        (processes[scheduler_current_process].state == process_state_running ||
         processes[scheduler_current_process].state == process_state_waiting)
    ) {
        memcpy(&processes[scheduler_current_process].cpu, cpu, sizeof(cpu_state));
    }
}

bool scheduler_idle_if_needed(cpu_state** cpu, struct vm_table** context) {
    if(processes[scheduler_current_process].state != process_state_runnable &&
        processes[scheduler_current_process].state != process_state_running) {
        *context = VM_KERNEL_CONTEXT;
        (*cpu)->rip    = (uint64_t)idle_task;
        (*cpu)->cs     = 0x2B;
        (*cpu)->ss     = 0x23;
        (*cpu)->rflags = 0x200;

        scheduler_current_process = -1;

        return true;
    }

    return false;
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

    if(scheduler_idle_if_needed(cpu, context)) {
        return;
    }

    processes[scheduler_current_process].state = process_state_running;
    *cpu     = &processes[scheduler_current_process].cpu;
    *context =  processes[scheduler_current_process].context;
}

void scheduler_process_cleanup(pid_t pid) {
    mutex_unlock_holder(pid);

    if(processes[pid].parent >= 0) {
        process_t* parent = &processes[processes[pid].parent];

        if(parent->state != process_state_empty  &&
           parent->state != process_state_exited &&
           parent->state != process_state_killed
        ) {
            size_t user_size = sizeof(struct SignalUserData);
            size_t size      = sizeof(struct Message) + user_size;

            struct Message* signal = vm_alloc(size);
            signal->size                    = size;
            signal->user_size               = user_size,
            signal->sender                  = -1,
            signal->type                    = MT_Signal,
            signal->user_data.Signal.signal = SIGCHLD,

            mq_push(parent->mq, signal);

            vm_free(signal);
        }
    }

    mq_destroy(processes[pid].mq);
}

void scheduler_kill_current(enum kill_reason reason) {
    processes[scheduler_current_process].state     = process_state_killed;
    processes[scheduler_current_process].exit_code = (int)reason;
    logd("scheduler", "'%s' (PID %d) killed for reason: %d)", processes[scheduler_current_process].name, scheduler_current_process, (int)reason);

    scheduler_process_cleanup(scheduler_current_process);
}

void sc_handle_scheduler_exit(uint64_t exit_code) {
    processes[scheduler_current_process].state     = process_state_exited;
    processes[scheduler_current_process].exit_code = exit_code;
    logd("scheduler", "'%s' (PID %d) exited (status: %d)", processes[scheduler_current_process].name, scheduler_current_process, exit_code);

    scheduler_process_cleanup(scheduler_current_process);
}

void sc_handle_scheduler_clone(bool share_memory, ptr_t entry, pid_t* newPid) {
    process_t* old = &processes[scheduler_current_process];

    // make new process
    pid_t pid = setup_process();

    if(pid < 0) {
        *newPid = -ENOMEM;
        return;
    }

    process_t* new = &processes[pid];
    strncpy(new->name, old->name, 1023);

    // new memory context ...
    new->context = vm_context_new();

    new->heap.start  = old->heap.start;
    new->heap.end    = old->heap.end;
    new->stack.start = old->stack.start;
    new->stack.end   = old->stack.end;
    new->hw.start    = old->hw.start;
    new->hw.end      = old->hw.end;

    // .. copy heap ..
    if(!share_memory) {
        vm_copy_range(new->context, old->context, old->heap.start, old->heap.end - old->heap.start);
    }
    else {
        // make heap shared
    }

    // .. and stack ..
    vm_copy_range(new->context, old->context, old->stack.start, old->stack.end - old->stack.start);

    // .. and remap hardware resources
    for(ptr_t i = old->hw.start; i < old->hw.end; i += 4096) {
        ptr_t hw = vm_context_get_physical_for_virtual(old->context, i);

        if(hw) {
            vm_context_map(new->context, i, hw, 0x07);
        }
    }

    if(share_memory && entry) {
        new->cpu.rip = entry;
    }

    *newPid = pid;

    // copy cpu state
    memcpy(&new->cpu, &old->cpu, sizeof(cpu_state));
    new->cpu.rax = 0;

    new->parent = scheduler_current_process;
}

bool scheduler_handle_pf(ptr_t fault_address, uint64_t error_code) {
    if(fault_address >= ALLOCATOR_REGION_USER_STACK.start && fault_address < ALLOCATOR_REGION_USER_STACK.end) {
        ptr_t page_v = fault_address & ~0xFFF;
        ptr_t page_p = (ptr_t)mm_alloc_pages(1);
        memset((void*)(page_p + ALLOCATOR_REGION_DIRECT_MAPPING.start), 0, 0x1000);
        vm_context_map(processes[scheduler_current_process].context, page_v, page_p, 0);

        if(page_v < processes[scheduler_current_process].stack.start) {
            processes[scheduler_current_process].stack.start = page_v;
        }

        return true;
    }

    logw("scheduler", "Not handling page fault for %s (PID %d) at 0x%x (RIP: 0x%x, error 0x%x)", processes[scheduler_current_process].name, scheduler_current_process, fault_address, processes[scheduler_current_process].cpu.rip, error_code);

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
                case wait_reason_message:
                    if(p->waiting_data.message_queue == data.message_queue) {
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
        for(ptr_t i = old_end & ~0xFFF; i < new_end; i += 0x1000) {
            if(!vm_context_get_physical_for_virtual(processes[scheduler_current_process].context, i)) {
                ptr_t phys = (ptr_t)mm_alloc_pages(1);
                memset((void*)(phys + ALLOCATOR_REGION_DIRECT_MAPPING.start), 0, 0x1000);
                vm_context_map(processes[scheduler_current_process].context, i, phys, 0);
            }
        }
    }
    if(inc < 0) {
        for(ptr_t i = old_end; i > new_end; i -= 0x1000) {
            if(!(i & ~0xFFF)) {
                mm_mark_physical_pages(vm_context_get_physical_for_virtual(processes[scheduler_current_process].context, i), 1, MM_FREE);
                vm_context_unmap(processes[scheduler_current_process].context, i);
            }
        }
    }

    processes[scheduler_current_process].heap.end = new_end;
    *data_end = old_end;
}

void sc_handle_scheduler_yield() {
    // no-op
}

void sc_handle_hardware_ioperm(uint16_t from, uint16_t num, bool turn_on, uint16_t* error) {
    *error = 0;
    process_t* process = &processes[scheduler_current_process];

    if(!process->iopb) {
        if(!turn_on) {
            return;
        }

        process->iopb = (ptr_t)mm_alloc_pages(2);
        set_iopb(process->context, process->iopb);
        memset((void*)(ALLOCATOR_REGION_DIRECT_MAPPING.start + process->iopb), 0xFF, 8*KiB);
    }

    bitmap_t bitmap = (bitmap_t)(ALLOCATOR_REGION_DIRECT_MAPPING.start + process->iopb);
    for(size_t i = 0; i < num; ++i) {
        if(turn_on) {
            bitmap_clear(bitmap, from + i);
        }
        else {
            bitmap_set(bitmap, from + i);
        }
    }

    if(!turn_on) {
        bool some_enabled = false;

        for(size_t i = 0; i < 8*KiB; ++i) {
            if(((uint8_t*)bitmap)[i] != 0xFF) {
                some_enabled = true;
                break;
            }
        }

        if(!some_enabled) {
            mm_mark_physical_pages(process->iopb, 2, MM_FREE);
            process->iopb = 0;
            set_iopb(process->context, process->iopb);
        }
    }
}

void sc_handle_hardware_interrupt_notify(uint8_t interrupt, bool enable, uint64_t mq, uint64_t* error) {
    if(interrupt > 16) {
        *error = ENOENT;
        return;
    }

    if(!mq) {
        mq = processes[scheduler_current_process].mq;
    }

    if(enable) {
        interrupt_add_queue(interrupt, mq);
    }
    else {
        interrupt_del_queue(interrupt, mq);
    }

    *error = 0;
}

void sc_handle_ipc_mq_create(bool global_read, bool global_write, size_t msg_limit, size_t data_limit, uint64_t* mq, uint64_t* error) {
    *error = 0;
}

void sc_handle_ipc_mq_destroy(uint64_t mq, uint64_t* error) {
    *error = 0;
}

void sc_handle_ipc_mq_poll(uint64_t mq, bool wait, struct Message* msg, uint64_t* error) {
    if(!mq) {
        mq = processes[scheduler_current_process].mq;
    }

    struct Message peeked = { .size = sizeof(struct Message) };
    *error = mq_peek(mq, &peeked);

    if(*error == ENOMSG && wait) {
        union wait_data data;
        data.message_queue = mq;
        scheduler_wait_for(scheduler_current_process, wait_reason_message, data);
        // this makes the syscall return EAGAIN as soon as a message is available,
        // making the process poll again and then receive the message.
        // TODO: implement a way to deliver syscall results when a process is
        //       scheduled the next time, so we don't have to poll twice
        *error = EAGAIN;
    }
    else if(*error == EMSGSIZE) {
        if(peeked.size > msg->size) {
            msg->size = peeked.size;
            msg->type = MT_Invalid;
        }
        else {
            *error = 0;
        }
    }

    if(*error) {
        return;
    }

    *error = mq_pop(mq, msg);
}

void sc_handle_ipc_mq_send(uint64_t mq, pid_t pid, struct Message* msg, uint64_t* error) {
    msg->sender = scheduler_current_process;

    if(!mq) {
        if(pid != -1) {
            if(pid < MAX_PROCS && processes[pid].state != process_state_empty) {
                mq = processes[pid].mq;
            }
        }
        else {
            mq = processes[scheduler_current_process].mq;
        }
    }

    if(mq) {
        *error = mq_push(mq, msg);
    }
}

void sc_handle_ipc_service_register(uuid_t* uuid, uint64_t mq, uint64_t* error) {
    if(!mq) {
        mq = processes[scheduler_current_process].mq;
    }

    *error = sd_register(uuid, mq);
}

void sc_handle_ipc_service_discover(uuid_t* uuid, uint64_t mq, struct Message* msg, uint64_t* error) {
    if(!mq) {
        mq = processes[scheduler_current_process].mq;
    }

    if(!msg || msg->type != MT_ServiceDiscovery) {
        *error = EINVAL;
        return;
    }

    msg->sender = scheduler_current_process;
    msg->user_data.ServiceDiscovery.mq = mq;
    memcpy(msg->user_data.ServiceDiscovery.serviceIdentifier.data, uuid, sizeof(uuid_t));

    int64_t delivered = sd_send(uuid, msg);

    if(delivered < 0) {
        *error = -delivered;
        return;
    }

    *error = 0;
}

void sc_handle_scheduler_get_pid(bool parent, pid_t* pid) {
    *pid = parent ? processes[scheduler_current_process].parent
                  : scheduler_current_process;
}

ptr_t scheduler_map_hardware(ptr_t hw, size_t len) {
    ptr_t res = vm_map_hardware(hw, len);

    if(res + len > processes[scheduler_current_process].hw.end) {
        processes[scheduler_current_process].hw.end = res + len;
    }

    return res;
}
