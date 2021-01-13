#include <string>
#include <kernel/syscalls.h>
#include <sys/io.h>

int main(int argc, char* argv[]) {
    uint64_t error;
    sc_do_hardware_ioperm(0x3F8, 7, true, &error);

    std::string message = "Hello world from userspace uart driver!";

    for(size_t i = 0; i < message.length(); ++i) {
        while((inb(0x3F8 + 5) & 0x20) == 0) {}
        outb(0x3F8, message[i]);
    }

    return 0;
}
