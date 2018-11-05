void putc(char c) {
    asm volatile("syscall"::"a"(c), "d"(42));
}

void puts(char* str) {
    while(*str++) {
        putc(*str);
    }
}

void _start() {
    asm volatile("cli\nhlt");
//    while(1);
//    puts("Hello world!\n");
    while(1);
}
