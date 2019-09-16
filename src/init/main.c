void exit(unsigned char exit_code) {
    asm volatile("syscall"::"a"(exit_code), "d"(0));
}

void _start() {
    exit(0);
}
