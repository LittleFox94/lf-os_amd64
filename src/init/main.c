#define NUM_FORKS 8

typedef unsigned long long uint64_t;

__attribute__((noinline)) void print(char* msg) {
    asm volatile("syscall"::"a"(msg), "d"(0xFF << 24):"rcx","r11");
}

__attribute__((noinline)) void exit(unsigned char exit_code) {
    asm volatile("syscall"::"a"(exit_code), "d"(0):"rcx","r11");
}

__attribute__((noinline)) volatile unsigned long long clone(unsigned char share, unsigned long long entry) {
    unsigned long long pid;
    asm volatile("syscall":"=a"(pid):"a"(share),"d"(1),"b"(entry):"rcx","r11");
    return pid;
}

__attribute__((noinline)) int main() {
    for(int i = 0; i < NUM_FORKS; ++i) {
        unsigned long long pid = clone(1, 0);
        print((char[]){'0' + i, 'A' + pid, 0});

        if(pid) {
            print("Forked!");
        }
        else {
            print("I'm a fork :o");
            return 0;
        }
    }

    return 0;
}

void _start() {
    int exit_code = main();
    while(1);
    exit(exit_code);
}
