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

#include <allocator/typed.h>
#include <memory/context.h>

#include <memory>

#define INVALID_PID (pid_t)-1

extern void sc_handle_clock_read(uint64_t* nanoseconds);

void* process_alloc(allocator_t* alloc, size_t size);
void process_dealloc(allocator_t* alloc, void* ptr);

typedef enum {
    process_state_empty = 0,
    process_state_waiting,
    process_state_runnable,
    process_state_running,
    process_state_exited,
    process_state_killed,
} process_state;

struct process_t {
    char name[128];

    pid_t         parent = INVALID_PID;
    uint8_t       exit_code;
    process_state state;

    enum wait_reason waiting_for;
    union wait_data  waiting_data;

    mq_id_t          mq;
    cpu_state        cpu;

    std::shared_ptr<MemoryContext> context;

    //! Physical address of the first of two IOPB pages, 0 if no IO privilege granted
    uint64_t iopb;

    allocator_t allocator;
    size_t allocatedMemory;

    process_t(pid_t pid, std::shared_ptr<MemoryContext> context) :
        context(context) {
        state       = process_state_runnable;
        cpu.cs      = 0x2B;
        cpu.ss      = 0x23;
        cpu.rflags  = 0x200;

        allocator.alloc   = process_alloc;
        allocator.dealloc = process_dealloc;
        allocator.tag     = pid;

        mq = mq_create(&allocator);
    }
};

pid_t scheduler_current_process = INVALID_PID;

// TODO: better data structure here. A binary tree with prev/next pointers should be great
#define MAX_PROCS 4096
static process_t* processes[MAX_PROCS];

TypedAllocator<process_t> process_allocator;

void* process_alloc(allocator_t* alloc, size_t size) {
    if(!alloc                                               ||
        processes[alloc->tag]->state == process_state_exited ||
        processes[alloc->tag]->state == process_state_killed ||
        processes[alloc->tag]->state == process_state_empty
    ) {
        logw("scheduler", "process_alloc called for invalid process %u\n", alloc->tag);
        return 0;
    }

    processes[alloc->tag]->allocatedMemory += size;

    return vm_alloc(size);
}

void process_dealloc(allocator_t* alloc, void* ptr) {
    if(!alloc || processes[alloc->tag]->state == process_state_empty) {
        logw("scheduler", "process_dealloc called for invalid process %u\n", alloc->tag);
        return;
    }

    size_t size = *((size_t*)ptr - 1);
    vm_free(ptr);

    processes[alloc->tag]->allocatedMemory -= size;
}

void init_scheduler(void) {
    memset((uint8_t*)processes, 0, sizeof(processes));
}

static process_t* current_process() {
    if (scheduler_current_process >= 0 &&
        scheduler_current_process <= sizeof(processes) / sizeof(processes[0])
    ) {
        return processes[scheduler_current_process];
    }

    return 0;
}

__attribute__((naked)) static void idle_task(void) {
    asm("jmp .");
}

pid_t allocate_pid(void) {
    pid_t i;
    for(i = 0; i < MAX_PROCS; ++i) {
        if(processes[i] == nullptr) {
            return i;
        }
    }

    return INVALID_PID;
}

pid_t setup_process(std::shared_ptr<MemoryContext> context) {
    pid_t pid = allocate_pid();
    if(pid == INVALID_PID) {
        panic_message("Out of PIDs!");
    }

    auto process = process_allocator.allocate(1);
    processes[pid] = process;
    process_allocator.construct(process, pid, context);

    return pid;
}

void start_task(std::shared_ptr<MemoryContext> context, uint64_t entry, const char* name) {
    if(!entry) {
        panic_message("Tried to start process without entry");
    }

    auto pid = setup_process(context);
    auto process = processes[pid];
    process->parent  = INVALID_PID;
    process->context = context;
    process->cpu.rip = entry;
    process->cpu.rsp = ALLOCATOR_REGION_USER_STACK.end;

    strncpy(process->name, name, sizeof(process_t::name));
}

void scheduler_process_save(cpu_state* cpu) {
    auto process = current_process();
    if (process &&
        (process->state == process_state_running ||
         process->state == process_state_waiting)
    ) {
        memcpy(&process->cpu, cpu, sizeof(cpu_state));
    }
}

bool scheduler_idle_if_needed(cpu_state** cpu, struct vm_table** context) {
    auto process = current_process();
    if (!process || (
            process->state != process_state_runnable &&
            process->state != process_state_running
        )
    ) {
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
    auto process = current_process();
    if (process &&
        processes[scheduler_current_process]->state == process_state_running
    ) {
        processes[scheduler_current_process]->state = process_state_runnable;
    }

    uint64_t timestamp_ns_since_boot = 0;

    static pid_t last_scheduled = INVALID_PID;
    for(int i = 1; i <= MAX_PROCS; ++i) {
        pid_t pid = (last_scheduled + i) % MAX_PROCS;

        process_t* process = processes[pid];
        if(!process) {
            continue;
        }

        if(process->state == process_state_waiting && process->waiting_for == wait_reason_time) {
            if(process->waiting_data.timestamp_ns_since_boot && !timestamp_ns_since_boot) {
                sc_handle_clock_read(&timestamp_ns_since_boot); // the kernel calling a syscall handler ... oh deer, but why not? :>
            }

            if(process->waiting_data.timestamp_ns_since_boot <= timestamp_ns_since_boot) {
                process->state = process_state_runnable;
            }
        }

        if(process->state == process_state_runnable) {
            last_scheduled = scheduler_current_process = pid;

            process->state = process_state_running;
            *cpu     = &process->cpu;
            *context =  static_cast<vm_table*>(process->context->arch_context());
            return;
        }
    }

    if(scheduler_idle_if_needed(cpu, context)) {
        return;
    }

    panic_message("Shouldn't have reached this code!");
}

bool schedule_next_if_needed(cpu_state** cpu, struct vm_table** context) {
    auto process = current_process();
    if (process &&
        process->state != process_state_runnable &&
        process->state != process_state_running
    ) {
        schedule_next(cpu, context);
        return true;
    }

    return false;
}


void scheduler_process_cleanup(pid_t pid) {
    mutex_unlock_holder(pid);

    auto process = processes[pid];
    if(process->parent != INVALID_PID) {
        auto parent = processes[process->parent];

        if (parent->state != process_state_empty  &&
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

    mq_destroy(processes[pid]->mq);

    delete process;
    processes[pid] = 0;
}

void scheduler_kill_current(enum kill_reason reason) {
    auto process = current_process();
    process->state     = process_state_killed;
    process->exit_code = (int)reason;
    logd("scheduler", "'%s' (PID %d) killed for reason: %d)", processes[scheduler_current_process]->name, scheduler_current_process, (int)reason);

    scheduler_process_cleanup(scheduler_current_process);
}

void sc_handle_scheduler_exit(uint8_t exit_code) {
    auto process = current_process();
    process->state     = process_state_exited;
    process->exit_code = exit_code;
    logd("scheduler", "'%s' (PID %d) exited (status: %d)", processes[scheduler_current_process]->name, scheduler_current_process, exit_code);

    scheduler_process_cleanup(scheduler_current_process);
}

void sc_handle_scheduler_clone(bool share_memory, void* entry, pid_t* newPid) {
    process_t* old = current_process();

    auto new_context = old->context;
    if(!share_memory) {
        new_context = std::make_shared<MemoryContext>(*new_context);
    }

    // make new process
    pid_t pid = setup_process(new_context);

    if(pid < 0) {
        *newPid = -ENOMEM;
        return;
    }

    process_t* new_process = processes[pid];
    strncpy(new_process->name, old->name, sizeof(process_t::name));

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

    //if(fault_address >= ALLOCATOR_REGION_USER_STACK.start && fault_address < ALLOCATOR_REGION_USER_STACK.end) {
    //    uint64_t page_v = fault_address & ~0xFFF;
    //    uint64_t page_p = (uint64_t)mm_alloc_pages(1);
    //    memset((void*)(page_p + ALLOCATOR_REGION_DIRECT_MAPPING.start), 0, 0x1000);
    //    vm_context_map(processes[scheduler_current_process]->context, page_v, page_p, 0);

    //    if(page_v < processes[scheduler_current_process].stack.start) {
    //        processes[scheduler_current_process]->stack.start = page_v;
    //    }

    //    return true;
    //}

    auto process = current_process();
    if(!process) {
        return false;
    }

    MemoryFault fault;
    fault.present = (error_code &  1) != 0;
    fault.access  = (error_code &  2) != 0 ? MemoryAccess::Write
                  : (error_code & 16) != 0 ? MemoryAccess::Execute
                  :                          MemoryAccess::Read;

    if(!process->context->handle_fault(fault_address, fault)) {
        logw("scheduler", "Not handling page fault for %s (PID %d) at 0x%x (RIP: 0x%x, error 0x%x)", process->name, scheduler_current_process, fault_address, process->cpu.rip, error_code);
        return false;
    }

    return true;
}

void scheduler_wait_for(pid_t pid, enum wait_reason reason, union wait_data data) {
    if(pid == INVALID_PID) {
        pid = scheduler_current_process;
    }

    processes[pid]->state        = process_state_waiting;
    processes[pid]->waiting_for  = reason;
    processes[pid]->waiting_data = data;
}

void scheduler_waitable_done(enum wait_reason reason, union wait_data data, size_t max_amount) {
    for(int i = 0; i < MAX_PROCS; ++i) {
        pid_t pid = (scheduler_current_process + i) % MAX_PROCS;

        process_t* p = processes[pid];

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
    UNUSED_PARAM(inc);
    *data_end = 0;
    //uint64_t old_end = processes[scheduler_current_process].heap.end;
    //uint64_t new_end = old_end + inc;

    //if(inc > 0) {
    //    for(uint64_t i = old_end & ~0xFFF; i < new_end; i += 0x1000) {
    //        if(!vm_context_get_physical_for_virtual(processes[scheduler_current_process].context, i)) {
    //            uint64_t phys = (uint64_t)mm_alloc_pages(1);
    //            memset((void*)(phys + ALLOCATOR_REGION_DIRECT_MAPPING.start), 0, 0x1000);
    //            vm_context_map(processes[scheduler_current_process].context, i, phys, 0);
    //        }
    //    }
    //}
    //if(inc < 0) {
    //    for(uint64_t i = old_end; i > new_end; i -= 0x1000) {
    //        if(!(i & ~0xFFF)) {
    //            mm_mark_physical_pages(vm_context_get_physical_for_virtual(processes[scheduler_current_process].context, i), 1, MM_FREE);
    //            vm_context_unmap(processes[scheduler_current_process].context, i);
    //        }
    //    }
    //}

    //processes[scheduler_current_process].heap.end = new_end;
    //*data_end = (void*)old_end;
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
    auto process = current_process();

    if(!process->iopb) {
        if(!turn_on) {
            return;
        }

        process->iopb = (uint64_t)mm_alloc_pages(2);
        //set_iopb(process->context, process->iopb);
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
            //set_iopb(process->context, process->iopb);
        }
    }
}

void sc_handle_hardware_interrupt_notify(uint8_t interrupt, bool enable, uint64_t mq, uint64_t* error) {
    if(interrupt > 16) {
        *error = ENOENT;
        return;
    }

    if(!mq) {
        mq = current_process()->mq;
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
        mq = current_process()->mq;
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
            if(pid < MAX_PROCS && processes[pid]->state != process_state_empty) {
                mq = processes[pid]->mq;
            }
        }
        else {
            mq = current_process()->mq;
        }
    }

    if(mq) {
        *error = mq_push(mq, msg);
    }
}

void sc_handle_ipc_service_register(const uuid_t* uuid, uint64_t mq, uint64_t* error) {
    if(!mq) {
        mq = current_process()->mq;
    }

    *error = sd_register(uuid, mq);
}

void sc_handle_ipc_service_discover(const uuid_t* uuid, uint64_t mq, struct Message* msg, uint64_t* error) {
    if(!mq) {
        mq = current_process()->mq;
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
    *pid = parent ? current_process()->parent
                  : scheduler_current_process;
}

uint64_t scheduler_map_hardware(uint64_t hw, size_t len) {
    UNUSED_PARAM(hw);
    UNUSED_PARAM(len);
    return 0;
    //uint64_t res = vm_map_hardware(hw, len);

    //if(res + len > processes[scheduler_current_process].hw.end) {
    //    processes[scheduler_current_process].hw.end = res + len;
    //}

    //return res;
}
