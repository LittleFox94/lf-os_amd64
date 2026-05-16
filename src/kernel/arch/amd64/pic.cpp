
#include <stdint.h>

#include "pic.h"
#include "io.h"

#define PIC1 0x20
#define PIC2 0xA0

static inline void pic_send_cmd(uint16_t port, uint8_t cmd) {
    outb(port, cmd);
    for (int i = 0; i < 5; i++) { asm volatile("nop"); }
}

void init_pic(void) {
	pic_send_cmd(PIC1,     0x11);  // starts the initialization sequence (in cascade mode)
	pic_send_cmd(PIC2,     0x11);
	pic_send_cmd(PIC1 + 1, 0x20);  // ICW2: Master PIC vector offset
	pic_send_cmd(PIC2 + 1, 0x28);  // ICW2: Slave PIC vector offset
	pic_send_cmd(PIC1 + 1, 4);     // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	pic_send_cmd(PIC2 + 1, 2);     // ICW3: tell Slave PIC its cascade identity (0000 0010)
 
	pic_send_cmd(PIC1 + 1, 1);
	pic_send_cmd(PIC2 + 1, 1);

    // Unmask all IRQs, found some in the wild as masked.
    // Initially we only unmasked IRQ0 to get the userspace running, but by now
    // the default userspace programs rely on other IRQs as well (keyboard,
    // uart), so let's just unmask all of them.
    pic_send_cmd(PIC1 + 1, 0);
    pic_send_cmd(PIC2 + 1, 0);
}

void pic_set_handled(int interrupt) {
    interrupt -= 0x20;

    pic_send_cmd(PIC1, 0x20);
    if(interrupt > 7) {
        pic_send_cmd(PIC2, 0x20);
    }
}

uint16_t pic_get_isr(void) {
    pic_send_cmd(PIC1, ((1 << 6) | (1 << 3)));
    pic_send_cmd(PIC2, ((1 << 6) | (1 << 3)));
    uint16_t ret = inb(PIC2) << 8;
    ret |= inb(PIC1);
    return ret;
}

