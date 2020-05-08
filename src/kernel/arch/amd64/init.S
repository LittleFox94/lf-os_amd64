.global _start
.type   _start, @function

_start:
	mov  $0, %rbp
	pushq $0 # align stack?
    callq main
	mov  mainReturnedMessage(%rip), %rsi
	callq panic_message

.global load_cr3
.type   load_cr3, @function

load_cr3:
    mov %rdi, %cr3
    retq

.type mainReturnedMessage, @object
mainReturnedMessage: .asciz "main() returned!"
