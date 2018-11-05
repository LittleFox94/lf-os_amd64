; some init helpers
; vim: ft=nasm:

bits 64

global jump_higher_half
global load_cr3

jump_higher_half:
	; we are called from relocate_high() in main.c but we end that function
	; here instead of jump_higher_half so efi_main() from main.c is then
	; running in higher half
	leave

	pop rax
	sub rax, rsi
	add rax, rdi

	push rax
	ret

load_cr3:
	mov cr3, rdi
	ret
