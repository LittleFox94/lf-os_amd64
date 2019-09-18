void exit(unsigned char exit_code) {
    asm volatile("syscall"::"a"(exit_code), "d"(0));
}

void print(char* msg) {
    asm volatile("syscall"::"a"(msg), "d"(0xFF << 24));
}

void _start() {
    print("Hello world from userspace!");
    while(1);
    exit(0);
}
