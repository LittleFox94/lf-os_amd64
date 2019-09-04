void putc(char c) {
    asm volatile("syscall"::"a"(c), "d"(42));
}

void puts(char* str) {
    while(*str++) {
        putc(*str);
    }
}

void _start() {
    puts("Hello world from userspace!\n");
    while(1);
}
