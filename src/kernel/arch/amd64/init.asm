; some init helpers
; vim: ft=nasm:

bits 64

global jump_higher_half
global load_cr3

jump_higher_half:
	pop rax
	sub rax, rsi
	add rax, rdi

	push rax
	ret

load_cr3:
	mov cr3, rdi
	ret
