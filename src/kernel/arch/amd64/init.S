.global load_cr3
.type   load_cr3, @function

load_cr3:
    mov %rdi, %cr3
    ret
