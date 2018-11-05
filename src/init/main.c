void _start() {
    asm volatile("syscall");
    asm volatile("syscall");
    asm volatile("syscall");
    while(1);
}
