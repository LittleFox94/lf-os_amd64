.global jump_higher_half
.global load_cr3

load_cr3: 
        mov %rdi, %cr3
        ret
