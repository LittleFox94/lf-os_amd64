#define NUM_FORKS 10

void exit(unsigned char exit_code) {
    asm volatile("syscall"::"a"(exit_code), "d"(0));
}

unsigned long long clone(unsigned char share, unsigned long long child_stack, unsigned long long entry) {
    unsigned long long pid;
    asm volatile("syscall":"=a"(pid):"a"(share), "d"(1), "S"(child_stack), "D"(entry));
    return pid;
}

void print(char* msg) {
    asm volatile("syscall"::"a"(msg), "d"(0xFF << 24));
}

void _start() {
    char stack[0x1000 * NUM_FORKS];

    for(int i = 1; i <= NUM_FORKS; ++i) {
        if(clone(1, stack + (i * 0x1000), 0)) {
            print("Forked!");
        }
        else {
            print("I'm a fork :o");
            exit(0);
        }
    }

    while(1);
    exit(0);
}
