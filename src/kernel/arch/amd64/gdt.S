.global reload_cs
.global _syscall_handler
.extern syscall_handler
.extern interrupt_handler

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

.macro __idt_handler_error_code e 
.global idt_entry_\e
idt_entry_\e: 
    cli
    push $\e
	jmp _interrupt_handler_common
.endm

.macro __idt_handler e
.global idt_entry_\e
idt_entry_\e:
    cli
    push $0
    push $\e
	jmp _interrupt_handler_common
.endm

_interrupt_handler_common:
    __pusha

    movq %rsp,%rdi
    call interrupt_handler
    movq %rax,%rsp

	__popa
    addq $16,%rsp /* remove error code from stack */

    sti
    iretq

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

reload_cs: 
	sub $16, %rsp
	movq $8, 8(%rsp)
	movabsq $_reload_cs, %rax
	mov %rax, (%rsp)
	lretq
	_reload_cs:
			mov $0x10,%cx
			mov %cx,%ds
			mov %cx,%es
			mov %cx,%fs
			mov %cx,%gs
			mov %cx,%ss
			ret

_syscall_handler: 
    pushq $0            # ss, doesn't matter here
        pushq %rsp
        pushf
        pushq $0        # cs, doesn't matter here
        pushq %rcx      # user rip
        pushq $0        # bogus interrupt
        pushq $0        # bogus errorcode

        __pusha
        movq %rsp,%rdi
        call syscall_handler
        movq %rax,%rsp
        __popa

        addq $16,%rsp # ignore bogus error and interrupt
        popq %rcx       # user rip
        addq $32,%rsp # ignore cs, flags, rsp, ss

        sti
        sysretq

