.global _start
.type   _start, @function
_start:
  mov $0, %rax
  push %rax
  push %rax
  mov %rsp, %rbp

  push %rdi
  push %rsi

  mov $__libc_fini_array, %rdi
  call atexit

  call __libc_init_array

  pop %rsi
  pop %rdi

  call main
  mov %rax, %rdi
  call exit

.global _init
.type   _init, @function
_init:
  ret

.global _fini
.type   _fini, @function
_fini:
  ret
