#include "sc.h"
#include "msr.h"
#include "string.h"
#include "mm.h"
#include "cpu.h"
#include "scheduler.h"
#include "pic.h"
#include "panic.h"
#include "vm.h"
#include "mq.h"
#include "flexarray.h"
#include "errno.h"

#define GDT_ACCESSED   0x01
#define GDT_RW         0x02
#define GDT_CONFORMING 0x04
#define GDT_EXECUTE    0x08
#define GDT_SYSTEM     0x10
#define GDT_RING1      0x20
#define GDT_RING2      0x40
#define GDT_RING3      0x60
#define GDT_PRESENT    0x80

struct idt_entry {
    uint16_t baseLow;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t baseMid;
    uint32_t baseHigh;
    uint32_t _reserved;
}__attribute__((packed));

struct gdt_entry {
    uint16_t limitLow;
    uint16_t baseLow;
    uint8_t  baseMid;
    uint8_t  type;
    uint8_t  size;
    uint8_t  baseHigh;
}__attribute__((packed));

struct table_pointer {
    uint16_t limit;
    uint64_t base;
}__attribute__((packed));

struct tss {
    uint32_t _reserved0;

    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;

    uint64_t _reserved1;

    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;

    uint64_t _reserved2;
    uint16_t _reserved3;
    uint16_t iopb_offset;
}__attribute__((packed));

typedef struct {
    uint64_t      kernel_stack;
    struct tss tss;
} cpu_local_data;

extern void sc_handle(cpu_state* cpu);

extern "C" void _syscall_handler(void);
extern "C" void reload_cs(void);

extern "C" void idt_entry_0(void);
extern "C" void idt_entry_1(void);
extern "C" void idt_entry_2(void);
extern "C" void idt_entry_3(void);
extern "C" void idt_entry_4(void);
extern "C" void idt_entry_5(void);
extern "C" void idt_entry_6(void);
extern "C" void idt_entry_7(void);
extern "C" void idt_entry_8(void);
extern "C" void idt_entry_9(void);
extern "C" void idt_entry_10(void);
extern "C" void idt_entry_11(void);
extern "C" void idt_entry_12(void);
extern "C" void idt_entry_13(void);
extern "C" void idt_entry_14(void);
extern "C" void idt_entry_15(void);
extern "C" void idt_entry_16(void);
extern "C" void idt_entry_17(void);
extern "C" void idt_entry_18(void);
extern "C" void idt_entry_19(void);
extern "C" void idt_entry_20(void);
extern "C" void idt_entry_21(void);
extern "C" void idt_entry_22(void);
extern "C" void idt_entry_23(void);
extern "C" void idt_entry_24(void);
extern "C" void idt_entry_25(void);
extern "C" void idt_entry_26(void);
extern "C" void idt_entry_27(void);
extern "C" void idt_entry_28(void);
extern "C" void idt_entry_29(void);
extern "C" void idt_entry_30(void);
extern "C" void idt_entry_31(void);
extern "C" void idt_entry_32(void);
extern "C" void idt_entry_33(void);
extern "C" void idt_entry_34(void);
extern "C" void idt_entry_35(void);
extern "C" void idt_entry_36(void);
extern "C" void idt_entry_37(void);
extern "C" void idt_entry_38(void);
extern "C" void idt_entry_39(void);
extern "C" void idt_entry_40(void);
extern "C" void idt_entry_41(void);
extern "C" void idt_entry_42(void);
extern "C" void idt_entry_43(void);
extern "C" void idt_entry_44(void);
extern "C" void idt_entry_45(void);
extern "C" void idt_entry_46(void);
extern "C" void idt_entry_47(void);

static cpu_local_data*  _cpu0;
static struct idt_entry _idt[256];

static flexarray_t interrupt_queues[16] = { 0 };

void set_iopb(struct vm_table* context, uint64_t new_iopb) {
    static uint64_t originalPages[2] = {0, 0};

    uint64_t iopb = (uint64_t)&_cpu0->tss + _cpu0->tss.iopb_offset;

    if(!originalPages[0] && !originalPages[1]) {
        originalPages[0] = vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, iopb);
        originalPages[1] = vm_context_get_physical_for_virtual(VM_KERNEL_CONTEXT, iopb + 4*KiB);
    }

    if(!new_iopb) {
        vm_context_map(context, ALLOCATOR_REGION_USER_IOPERM.start,         originalPages[0], 0);
        vm_context_map(context, ALLOCATOR_REGION_USER_IOPERM.start + 4*KiB, originalPages[1], 0);
    }
    else {
        vm_context_map(context, ALLOCATOR_REGION_USER_IOPERM.start,         new_iopb, 0);
        vm_context_map(context, ALLOCATOR_REGION_USER_IOPERM.start + 4*KiB, new_iopb * 4*KiB, 0);
    }
}

void init_gdt(void) {
    _cpu0 = (cpu_local_data*)vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, 4);
    memset(_cpu0, 0, 4*KiB);
    memset((char*)_cpu0 + 4*KiB, 0xFF, 12*KiB);

    uint64_t kernel_stack = vm_context_alloc_pages(VM_KERNEL_CONTEXT, ALLOCATOR_REGION_KERNEL_HEAP, 1) + 4096;

    _cpu0->tss._reserved1  = 0;
    _cpu0->tss._reserved2  = 0;
    _cpu0->tss._reserved3  = 0;
    _cpu0->tss.iopb_offset = 0x1000 - ((uint64_t)&_cpu0->tss & 0xFFF);
    _cpu0->tss.ist1 = _cpu0->tss.rsp0 = _cpu0->tss.rsp1 = _cpu0->tss.rsp2 = _cpu0->kernel_stack = kernel_stack;

    static struct gdt_entry gdt[8];
    memset((uint8_t*)gdt,  0, sizeof(gdt));

    // kernel CS
    gdt[1].type = GDT_SYSTEM | GDT_PRESENT | GDT_RW | GDT_EXECUTE;
    gdt[1].size = 0xa0;

    // kernel SS
    gdt[2].type = GDT_SYSTEM | GDT_PRESENT | GDT_RW;
    gdt[2].size = 0xa0;

    // user CS (compatibility mode)
    // XXX: make 32bit descriptor
    gdt[3].type = GDT_SYSTEM | GDT_PRESENT | GDT_RW | GDT_EXECUTE | GDT_RING3;
    gdt[3].size = 0xa0;

    // user SS
    gdt[4].type = GDT_SYSTEM | GDT_PRESENT | GDT_RW | GDT_RING3;
    gdt[4].size = 0xa0;

    // user CS (long mode)
    gdt[5].type = GDT_SYSTEM | GDT_PRESENT | GDT_RW | GDT_EXECUTE | GDT_RING3;
    gdt[5].size = 0xa0;

    // TSS
    gdt[6].type     = GDT_PRESENT | GDT_ACCESSED | GDT_EXECUTE;
    gdt[6].limitLow = sizeof(struct tss) + (8*KiB) + 1;
    gdt[6].baseLow  = ((uint64_t)&_cpu0->tss & 0xFFFF);
    gdt[6].baseMid  = ((uint64_t)&_cpu0->tss >> 16) & 0xFF;
    gdt[6].baseHigh = ((uint64_t)&_cpu0->tss >> 24) & 0xFF;
    gdt[7].type     = 0;
    gdt[7].limitLow = ((uint64_t)&_cpu0->tss >> 32) & 0xFFFF;
    gdt[7].baseLow  = ((uint64_t)&_cpu0->tss >> 48) & 0xFFFF;

    struct table_pointer gdtp = {
        .limit = sizeof(gdt) -1,
        .base  = (uint64_t)gdt,
    };

    asm("lgdt %0"::"m"(gdtp));
    reload_cs();
    asm("ltr %%ax"::"a"(6 << 3));
}

void interrupt_del_queue(uint8_t interrupt, uint64_t mq) {
    flexarray_t array;
    uint64_t idx;

    if((array = interrupt_queues[interrupt]) && (idx = flexarray_find(array, &mq)) != -1ULL) {
        flexarray_remove(array, idx);
    }
}

static void interrupt_delall_queue(uint64_t mq) {
    for(size_t i = 0; i < sizeof(interrupt_queues) / sizeof(interrupt_queues[0]); i++) {
        interrupt_del_queue(i, mq);
    }
}

void interrupt_add_queue(uint8_t interrupt, uint64_t mq) {
    flexarray_t array;

    if(!(array = interrupt_queues[interrupt])) {
        array = new_flexarray(sizeof(uint64_t), 0, &kernel_alloc);
        interrupt_queues[interrupt] = array;
    }

    int error;
    if((error = mq_notify_teardown(mq, interrupt_delall_queue)) && error != EEXIST) {
        logw("sc", "error adding message queue teardown notify while adding mq %llu to interrupt queues for interrupt %u: $llu", mq, (uint64_t)interrupt, error);
    }
    flexarray_append(array, &mq);
}

static void _set_idt_entry(int index, uint64_t base) {
    _idt[index].baseLow  = base         & 0xFFFF;
    _idt[index].baseMid  = (base >> 16) & 0xFFFF;
    _idt[index].baseHigh = base  >> 32;
    _idt[index].selector = 0x08;
    _idt[index].flags    = 0xEE;
    _idt[index].ist      = 1;
}

static void _setup_idt(void) {
    memset((uint8_t*)_idt, 0, sizeof(_idt));

    _set_idt_entry( 0, (uint64_t) idt_entry_0);
    _set_idt_entry( 1, (uint64_t) idt_entry_1);
    _set_idt_entry( 2, (uint64_t) idt_entry_2);
    _set_idt_entry( 3, (uint64_t) idt_entry_3);
    _set_idt_entry( 4, (uint64_t) idt_entry_4);
    _set_idt_entry( 5, (uint64_t) idt_entry_5);
    _set_idt_entry( 6, (uint64_t) idt_entry_6);
    _set_idt_entry( 7, (uint64_t) idt_entry_7);
    _set_idt_entry( 8, (uint64_t) idt_entry_8);
    _set_idt_entry( 9, (uint64_t) idt_entry_9);
    _set_idt_entry(10, (uint64_t)idt_entry_10);
    _set_idt_entry(11, (uint64_t)idt_entry_11);
    _set_idt_entry(12, (uint64_t)idt_entry_12);
    _set_idt_entry(13, (uint64_t)idt_entry_13);
    _set_idt_entry(14, (uint64_t)idt_entry_14);
    _set_idt_entry(15, (uint64_t)idt_entry_15);
    _set_idt_entry(16, (uint64_t)idt_entry_16);
    _set_idt_entry(17, (uint64_t)idt_entry_17);
    _set_idt_entry(18, (uint64_t)idt_entry_18);
    _set_idt_entry(19, (uint64_t)idt_entry_19);
    _set_idt_entry(20, (uint64_t)idt_entry_20);
    _set_idt_entry(21, (uint64_t)idt_entry_21);
    _set_idt_entry(22, (uint64_t)idt_entry_22);
    _set_idt_entry(23, (uint64_t)idt_entry_23);
    _set_idt_entry(24, (uint64_t)idt_entry_24);
    _set_idt_entry(25, (uint64_t)idt_entry_25);
    _set_idt_entry(26, (uint64_t)idt_entry_26);
    _set_idt_entry(27, (uint64_t)idt_entry_27);
    _set_idt_entry(28, (uint64_t)idt_entry_28);
    _set_idt_entry(29, (uint64_t)idt_entry_29);
    _set_idt_entry(30, (uint64_t)idt_entry_30);
    _set_idt_entry(31, (uint64_t)idt_entry_31);
    _set_idt_entry(32, (uint64_t)idt_entry_32);
    _set_idt_entry(33, (uint64_t)idt_entry_33);
    _set_idt_entry(34, (uint64_t)idt_entry_34);
    _set_idt_entry(35, (uint64_t)idt_entry_35);
    _set_idt_entry(36, (uint64_t)idt_entry_36);
    _set_idt_entry(37, (uint64_t)idt_entry_37);
    _set_idt_entry(38, (uint64_t)idt_entry_38);
    _set_idt_entry(39, (uint64_t)idt_entry_39);
    _set_idt_entry(40, (uint64_t)idt_entry_40);
    _set_idt_entry(41, (uint64_t)idt_entry_41);
    _set_idt_entry(42, (uint64_t)idt_entry_42);
    _set_idt_entry(43, (uint64_t)idt_entry_43);
    _set_idt_entry(44, (uint64_t)idt_entry_44);
    _set_idt_entry(45, (uint64_t)idt_entry_45);
    _set_idt_entry(46, (uint64_t)idt_entry_46);
    _set_idt_entry(47, (uint64_t)idt_entry_47);

    struct table_pointer idtp = {
        .limit = sizeof(_idt) - 1,
        .base  = (uint64_t)_idt,
    };
    asm("lidt %0"::"m"(idtp));
}

void init_sc(void) {
    _setup_idt();

    asm("mov $0xC0000080, %%rcx\n"
        "rdmsr\n"
        "or $1, %%rax\n"
        "wrmsr":::"rcx","rax");

    write_msr(0xC0000081, 0x001B000800000000);
    write_msr(0xC0000082, (uint64_t)_syscall_handler);
    write_msr(0xC0000084, 0x200); // disable interrupts on syscall
    write_msr(0xC0000102, (uint64_t)(_cpu0));
}

static void enable_iopb(struct vm_table* context) {
    uint64_t iopb_pages[2] = {
        vm_context_get_physical_for_virtual(context, ALLOCATOR_REGION_USER_IOPERM.start),
        vm_context_get_physical_for_virtual(context, ALLOCATOR_REGION_USER_IOPERM.start + 4*KiB),
    };

    uint64_t iopb = (uint64_t)&_cpu0->tss + _cpu0->tss.iopb_offset;

    vm_context_map(context, iopb,         iopb_pages[0], 0);
    vm_context_map(context, iopb + 4*KiB, iopb_pages[1], 0);

    asm("invlpg (%0)"::"r"(iopb));
    asm("invlpg (%0)"::"r"(iopb + (4*KiB)));
}

static cpu_state* schedule_process(cpu_state* old_cpu) {
    cpu_state*  new_cpu = old_cpu; // for idle task we only change some fields,
                                   // allocating a new cpu for that is ..
                                   // correct but slow, so we just reuse the old one

    struct vm_table* new_context;
    schedule_next(&new_cpu, &new_context);
    vm_context_activate(new_context);
    enable_iopb(new_context);

    return new_cpu;
}

static bool handle_userspace_exception(cpu_state* cpu) {
    if(cpu->interrupt == 0x0e) {
        uint64_t fault_address;
        asm("mov %%cr2, %0":"=r"(fault_address));

        if(scheduler_handle_pf(fault_address, cpu->error_code)) {
            return true;
        }
    }

    logw("sc", "Process %d caused exception %u for reason %u at 0x%x, cpu dump below", scheduler_current_process, cpu->interrupt,cpu->error_code, cpu->rip);
    DUMP_CPU(cpu);

    // exception in user space
    if(cpu->interrupt == 14) {
        scheduler_kill_current(kill_reason_segv);
    }
    else {
        scheduler_kill_current(kill_reason_abort);
    }

    return true;
}

__attribute__ ((force_align_arg_pointer))
extern "C" cpu_state* interrupt_handler(cpu_state* cpu) {
    scheduler_process_save(cpu);

    if(cpu->interrupt < 32) {
        if((cpu->rip & 0x0000800000000000) == 0) {
            if(handle_userspace_exception(cpu)) {
                cpu_state*       new_cpu     = cpu;
                struct vm_table* new_context = vm_current_context();

                if(schedule_next_if_needed(&new_cpu, &new_context)) {
                    vm_context_activate(new_context);
                }

                return new_cpu;
            }
        }

        panic_cpu(cpu);
    }
    else if(cpu->interrupt >= 32 && cpu->interrupt < 48) {
        pic_set_handled(cpu->interrupt);

        uint8_t irq = cpu->interrupt - 0x20;
        if(interrupt_queues[irq]) {
            size_t len             = flexarray_length(interrupt_queues[irq]);
            const uint64_t* queues = (uint64_t*)flexarray_getall(interrupt_queues[irq]);

            size_t user_size    = sizeof(Message::UserData::HardwareInterruptUserData);
            size_t size         = sizeof(Message) + user_size;
            Message* msg        = (Message*)vm_alloc(size);

            msg->size                                  = size;
            msg->user_size                             = user_size;
            msg->type                                  = MT_HardwareInterrupt;
            msg->sender                                = -1;
            msg->user_data.HardwareInterrupt.interrupt = irq;

            for(size_t i = 0; i < len; ++i) {
                mq_push(queues[i], msg);
            }

            vm_free(msg);
        }
    }

    return schedule_process(cpu);
}

__attribute__ ((force_align_arg_pointer))
extern "C" cpu_state* syscall_handler(cpu_state* cpu) {
    scheduler_process_save(cpu);
    sc_handle(cpu);
    scheduler_process_save(cpu);

    cpu_state*  new_cpu = cpu; // for idle task we only change some fields,
                               // allocating a new cpu for that is ..
                               // correct but slow, so we just reuse the old one
    struct vm_table* new_context = vm_current_context();

    if(schedule_next_if_needed(&new_cpu, &new_context)) {
        vm_context_activate(new_context);
    }

    enable_iopb(new_context);
    return new_cpu;
}
