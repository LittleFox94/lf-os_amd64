# LF OS amd64 syscalls

"This is going to be a long, boring document" -- Mara Sophie Grosch, 2019-09-09

## Invoking syscalls

Syscalls are generally invoked with the `syscall` instruction. The syscall number is placed in `rdx`,
arguments are in `rax`, `rbx`, `rsi`, `rdi`.

Return values are given in `rax` and `rbx`. Syscalls will only change register contents as documented.

## Where is all the file and socket stuff?

LF OS is based on a microkernel. File, sockets and other facilities are implemented in user space
processes and you have to find the right one with the service registry syscalls and do IPC. Syscalls
are for very low level, process level things (like memory, basic IPC, scheduling).

## Table of syscalls

| # | Name | Description            | Arguments           | Return values                               |
|---|------|------------------------|---------------------|---------------------------------------------|
| 0 | exit | Just exit the program  | `RAX`: exit code    | does not return                             |
| 1 | fork | Clone the process      | none                | 0 if new process, pid of new if old process |
| 2 | sbrk | Set break, adjust heap | `RAX`: new heap end | `RAX`: new heap end                         |
