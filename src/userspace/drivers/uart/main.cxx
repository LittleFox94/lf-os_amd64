#include <string>
#include <iostream>
#include <kernel/syscalls.h>
#include <sys/io.h>

static const uint16_t base_port = 0x3f8;

static inline void wait_transmitter_empty() {
    size_t size         = sizeof(struct Message) + sizeof(uint16_t);
    struct Message* msg = (struct Message*)malloc(size);
    msg->size = size;

    uint64_t error;

    do {
        sc_do_ipc_mq_poll(0, true, msg, &error);
    } while(
        error == EAGAIN ||
        (error == 0 && msg->type != Message::MT_HardwareInterrupt)
    );

    std::cerr << "Received IRQ " << msg->user_data.HardwareInterrupt.interrupt << " in process" << std::endl;

    free(msg);
}

int main(int argc, char* argv[]) {
    uint64_t error;
    sc_do_hardware_ioperm(base_port, 7, true, &error);
    sc_do_hardware_interrupt_notify(4, true, 0, &error);

    outb(base_port + 1, 0x02); // enable interrupts on transmitter empty

    std::string message = "Hello world from userspace uart driver!";

    for(size_t i = 0; i < message.length(); ++i) {
        outb(0x3F8, message[i]);
        wait_transmitter_empty();
    }

    return 0;
}
