.extern syscall_handler
.type   syscall_handler, @function

.extern interrupt_handler
.type   interrupt_handler, @function

.macro __pusha
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rbp
    pushq %rdi
    pushq %rsi
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm

.macro __popa
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rsi
    popq %rdi
    popq %rbp
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
.endm

.global reload_cs
.type   reload_cs, @function
reload_cs:
    sub $16,   %rsp
    movq $8, 8(%rsp)
    movabsq $_reload_cs, %rax
    mov %rax, (%rsp)
    lretq

    .type _reload_cs, @function
    _reload_cs:
        mov $0x10, %cx
        mov %cx,   %ds
        mov %cx,   %es
        mov %cx,   %ss
        mov %cx,   %fs
        mov %cx,   %gs

        ret

.global _interrupt_handler_common
.type   _interrupt_handler_common, @function
_interrupt_handler_common:
    cli

    __pusha
    mov  %rsp, %rdi
    call interrupt_handler
    mov  %rax, %rsp
    __popa

    add $16, %rsp /* remove error code and interrupt from stack */
    sti
    iretq

.global _syscall_handler
.type   _syscall_handler, @function
_syscall_handler:
    mov %rsp, %rbx
    swapgs
    mov %gs:0, %rsp

    pushq $0x23    # fake ss
    push %rbx      # push original stack pointer
    push %r11      # store flags
    pushq $0x2B    # fake cs
    push %rcx      # user rip

    pushq $0       # bogus error
    pushq $0       # bogus interrupt

    __pusha
    mov  %rsp, %rdi
    call syscall_handler
    mov  %rax, %rsp
    __popa

    add $16, %rsp  # ignore bogus error and interrupt

    # we return with iretq instead of sysret to not clobber registers in
    # the exit part of _syscall_handler, allowing us to activate a process
    # that was deactivated by a schedule while handling an interrupt.
    # In theory, it would be nice if we checked if another task is active
    # now and only use iretq in that case, but it apparently works in all
    # cases anyway
    swapgs
    iretq

# interrupt vectors

.macro __idt_handler_error_code e
.global idt_entry_\e
.type   idt_entry_\e, @function
idt_entry_\e:
    cli
    pushq $\e
    jmp _interrupt_handler_common
.endm

.macro __idt_handler e
.global idt_entry_\e
.type   idt_entry_\e, @function
idt_entry_\e:
    cli
    pushq $0
    pushq $\e
    jmp _interrupt_handler_common
.endm

__idt_handler             0
__idt_handler             1
__idt_handler             2
__idt_handler             3
__idt_handler             4
__idt_handler             5
__idt_handler             6
__idt_handler             7
__idt_handler_error_code  8
__idt_handler             9
__idt_handler_error_code 10
__idt_handler_error_code 11
__idt_handler_error_code 12
__idt_handler_error_code 13
__idt_handler_error_code 14
__idt_handler            15
__idt_handler            16
__idt_handler_error_code 17
__idt_handler_error_code 18
__idt_handler            19
__idt_handler            20
__idt_handler            21
__idt_handler            22
__idt_handler            23
__idt_handler            24
__idt_handler            25
__idt_handler            26
__idt_handler            27
__idt_handler            28
__idt_handler            29
__idt_handler_error_code 30
__idt_handler            31

__idt_handler            32
__idt_handler            33
__idt_handler            34
__idt_handler            35
__idt_handler            36
__idt_handler            37
__idt_handler            38
__idt_handler            39
__idt_handler            40
__idt_handler            41
__idt_handler            42
__idt_handler            43
__idt_handler            44
__idt_handler            45
__idt_handler            46
__idt_handler            47
