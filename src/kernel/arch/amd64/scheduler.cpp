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
#include <unused_param.h>

extern void sc_handle_clock_read(uint64_t* nanoseconds);

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
    uint64_t iopb;

    allocator_t allocator;
    size_t allocatedMemory;
} process_t;

#define INVALID_PID (pid_t)-1
pid_t scheduler_current_process = INVALID_PID;

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

__attribute__((naked)) static void idle_task(void) {
    asm("jmp .");
}

void init_scheduler(void) {
    memset((uint8_t*)processes, 0, sizeof(process_t) * MAX_PROCS);
}

pid_t free_pid(void) {
    pid_t i;
    for(i = 0; i < MAX_PROCS; ++i) {
        if(processes[i].state == process_state_empty) {
            return i;
        }
    }

    return INVALID_PID;
}

pid_t setup_process(void) {
    pid_t pid = free_pid();
    if(pid == INVALID_PID) {
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

    process->allocator.alloc   = process_alloc;
    process->allocator.dealloc = process_dealloc;
    process->allocator.tag     = pid;

    process->mq = mq_create(&process->allocator);

    return pid;
}

void start_task(struct vm_table* context, uint64_t entry, uint64_t data_start, uint64_t data_end, const char* name) {
    if(!entry) {
        panic_message("Tried to start process without entry");
    }

    pid_t pid          = setup_process();
    process_t* process = &processes[pid];

    process->parent  = INVALID_PID;
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

        scheduler_current_process = INVALID_PID;

        return true;
    }

    return false;
}

void schedule_next(cpu_state** cpu, struct vm_table** context) {
    if(scheduler_current_process >= 0 && processes[scheduler_current_process].state == process_state_running) {
        processes[scheduler_current_process].state = process_state_runnable;
    }

    uint64_t timestamp_ns_since_boot = 0;

    static pid_t last_scheduled = INVALID_PID;
    for(int i = 1; i <= MAX_PROCS; ++i) {
        pid_t pid = (last_scheduled + i) % MAX_PROCS;

        process_t* process = &processes[pid];

        if(process->state == process_state_waiting && process->waiting_for == wait_reason_time) {
            if(process->waiting_data.timestamp_ns_since_boot && !timestamp_ns_since_boot) {
                sc_handle_clock_read(&timestamp_ns_since_boot); // the kernel calling a syscall handler ... oh deer, but why not? :>
            }

            if(process->waiting_data.timestamp_ns_since_boot <= timestamp_ns_since_boot) {
                process->state = process_state_runnable;
            }
        }

        if(processes[pid].state == process_state_runnable) {
            scheduler_current_process = pid;
            break;
        }
    }

    if(scheduler_idle_if_needed(cpu, context)) {
        return;
    }

    last_scheduled = scheduler_current_process;

    processes[scheduler_current_process].state = process_state_running;
    *cpu     = &processes[scheduler_current_process].cpu;
    *context =  processes[scheduler_current_process].context;
}

bool schedule_next_if_needed(cpu_state** cpu, struct vm_table** context) {
    if(processes[scheduler_current_process].state != process_state_runnable &&
        processes[scheduler_current_process].state != process_state_running) {
        schedule_next(cpu, context);
        return true;
    }

    return false;
}


void scheduler_process_cleanup(pid_t pid) {
    mutex_unlock_holder(pid);

    if(processes[pid].parent >= 0) {
        process_t* parent = &processes[processes[pid].parent];

        if(parent->state != process_state_empty  &&
           parent->state != process_state_exited &&
           parent->state != process_state_killed
        ) {
            size_t user_size = sizeof(struct Message::UserData::SignalUserData);
            size_t size      = sizeof(struct Message) + user_size;

            Message* signal = (Message*)vm_alloc(size);
            signal->size                    = size;
            signal->user_size               = user_size,
            signal->sender                  = INVALID_PID,
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

void sc_handle_scheduler_exit(uint8_t exit_code) {
    processes[scheduler_current_process].state     = process_state_exited;
    processes[scheduler_current_process].exit_code = exit_code;
    logd("scheduler", "'%s' (PID %d) exited (status: %d)", processes[scheduler_current_process].name, scheduler_current_process, exit_code);

    scheduler_process_cleanup(scheduler_current_process);
}

void sc_handle_scheduler_clone(bool share_memory, void* entry, pid_t* newPid) {
    process_t* old = &processes[scheduler_current_process];

    // make new process
    pid_t pid = setup_process();

    if(pid < 0) {
        *newPid = -ENOMEM;
        return;
    }

    process_t* new_process = &processes[pid];
    strncpy(new_process->name, old->name, 1023);

    // new memory context ...
    new_process->context = vm_context_new();

    new_process->heap.start  = old->heap.start;
    new_process->heap.end    = old->heap.end;
    new_process->stack.start = old->stack.start;
    new_process->stack.end   = old->stack.end;
    new_process->hw.start    = old->hw.start;
    new_process->hw.end      = old->hw.end;

    // .. copy heap ..
    if(!share_memory) {
        vm_copy_range(new_process->context, old->context, old->heap.start, old->heap.end - old->heap.start);
    }
    else {
        // make heap shared
    }

    // .. and stack ..
    vm_copy_range(new_process->context, old->context, old->stack.start, old->stack.end - old->stack.start);

    // .. and remap hardware resources
    for(uint64_t i = old->hw.start; i < old->hw.end; i += 4096) {
        uint64_t hw = vm_context_get_physical_for_virtual(old->context, i);

        if(hw) {
            vm_context_map(new_process->context, i, hw, 0x07);
        }
    }

    if(share_memory && entry) {
        new_process->cpu.rip = (uint64_t)entry;
    }

    *newPid = pid;

    // copy cpu state
    memcpy(&new_process->cpu, &old->cpu, sizeof(cpu_state));
    new_process->cpu.rax = 0;

    new_process->parent = scheduler_current_process;
}

bool scheduler_handle_pf(uint64_t fault_address, uint64_t error_code) {
    if(fault_address >= ALLOCATOR_REGION_USER_STACK.start && fault_address < ALLOCATOR_REGION_USER_STACK.end) {
        uint64_t page_v = fault_address & ~0xFFF;
        uint64_t page_p = (uint64_t)mm_alloc_pages(1);
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
    if(pid == INVALID_PID) {
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
                case wait_reason_time:
                    // handled in schedule_next instead
                    break;
            }
        }
    }
}

void sc_handle_memory_sbrk(int64_t inc, void** data_end) {
    uint64_t old_end = processes[scheduler_current_process].heap.end;
    uint64_t new_end = old_end + inc;

    if(inc > 0) {
        for(uint64_t i = old_end & ~0xFFF; i < new_end; i += 0x1000) {
            if(!vm_context_get_physical_for_virtual(processes[scheduler_current_process].context, i)) {
                uint64_t phys = (uint64_t)mm_alloc_pages(1);
                memset((void*)(phys + ALLOCATOR_REGION_DIRECT_MAPPING.start), 0, 0x1000);
                vm_context_map(processes[scheduler_current_process].context, i, phys, 0);
            }
        }
    }
    if(inc < 0) {
        for(uint64_t i = old_end; i > new_end; i -= 0x1000) {
            if(!(i & ~0xFFF)) {
                mm_mark_physical_pages(vm_context_get_physical_for_virtual(processes[scheduler_current_process].context, i), 1, MM_FREE);
                vm_context_unmap(processes[scheduler_current_process].context, i);
            }
        }
    }

    processes[scheduler_current_process].heap.end = new_end;
    *data_end = (void*)old_end;
}

void sc_handle_scheduler_sleep(uint64_t nanoseconds) {
    union wait_data wait_data;
    wait_data.timestamp_ns_since_boot = 0;

    if(nanoseconds) {
        uint64_t current_timestamp = 0;
        sc_handle_clock_read(&current_timestamp); // the kernel calling a syscall handler ... oh deer, but why not? :>

        wait_data.timestamp_ns_since_boot = current_timestamp + nanoseconds;
    }

    scheduler_wait_for(scheduler_current_process, wait_reason_time, wait_data);
}

void sc_handle_hardware_ioperm(uint16_t from, uint16_t num, bool turn_on, uint64_t* error) {
    *error = 0;
    process_t* process = &processes[scheduler_current_process];

    if(!process->iopb) {
        if(!turn_on) {
            return;
        }

        process->iopb = (uint64_t)mm_alloc_pages(2);
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
    UNUSED_PARAM(global_read);
    UNUSED_PARAM(global_write);
    UNUSED_PARAM(msg_limit);
    UNUSED_PARAM(data_limit);
    UNUSED_PARAM(mq);

    *error = -ENOSYS;
}

void sc_handle_ipc_mq_destroy(uint64_t mq, uint64_t* error) {
    UNUSED_PARAM(mq);

    *error = -ENOSYS;
}

void sc_handle_ipc_mq_poll(uint64_t mq, bool wait, struct Message* msg, uint64_t* error) {
    if(!mq) {
        mq = processes[scheduler_current_process].mq;
    }

    struct Message peeked;
    peeked.size = sizeof(struct Message);
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
        if(pid != INVALID_PID) {
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

void sc_handle_ipc_service_register(const uuid_t* uuid, uint64_t mq, uint64_t* error) {
    if(!mq) {
        mq = processes[scheduler_current_process].mq;
    }

    *error = sd_register(uuid, mq);
}

void sc_handle_ipc_service_discover(const uuid_t* uuid, uint64_t mq, struct Message* msg, uint64_t* error) {
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

uint64_t scheduler_map_hardware(uint64_t hw, size_t len) {
    uint64_t res = vm_map_hardware(hw, len);

    if(res + len > processes[scheduler_current_process].hw.end) {
        processes[scheduler_current_process].hw.end = res + len;
    }

    return res;
}
