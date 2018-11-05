; vim: ft=nasm:
[bits 64]

global reload_cs
global _syscall_handler
extern syscall_handler

reload_cs:
	pop rbx ; save return address

	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov rsp, 0x1000

    mov qword [rsp+8], 0x08
    mov qword [rsp], rbx
    iretq

_syscall_handler:
	mov rsp, 0x1000

	push rcx
	push rsp

	call syscall_handler

	pop rsp
	pop rcx
	o64 sysret
