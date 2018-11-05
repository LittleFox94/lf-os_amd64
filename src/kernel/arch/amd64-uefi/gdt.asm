; vim: ft=nasm:
[bits 64]

global reload_cs
global _syscall_handler
extern syscall_handler
extern interrupt_handler

%macro __pusha 0
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro __popa 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

%macro __idt_handler_error_code 1
global idt_entry_%1
idt_entry_%1:
	cli
	push qword %1
	__pusha

	mov rdi, rsp
	call interrupt_handler
	mov rsp, rax

	__popa
	add rsp, 16

	sti
	iretq
%endmacro

%macro __idt_handler 1
global idt_entry_%1
idt_entry_%1:
	cli
	push qword 0
	push qword %1
	__pusha

	mov rdi, rsp
	call interrupt_handler
	mov rsp, rax

	__popa
	add rsp, 16 ; remove error code from stack

	sti
	iretq
%endmacro

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
	pop rbx ; save return address

	; save flags
	pushf
	pop rcx

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov rsp, 0x1000

    mov qword [rsp+8],  0x08
    mov qword [rsp],    rbx
	o64 retf

_syscall_handler:
    push 0		; ss, doesn't matter here
	push rsp
	pushf
	push 0		; cs, doesn't matter here
	push rcx	; user rip
	push 0		; bogus errorcode

	__pusha
	mov rdi, rsp
	call syscall_handler
	mov rsp, rax
	__popa

    add rsp, 16	; ignore bogus error
	pop rcx		; user rip
	add rsp, 32 ; ignore cs, flags, rsp, ss

	sti
	o64 sysret
