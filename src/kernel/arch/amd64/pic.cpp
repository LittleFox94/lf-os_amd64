#include "pic.h"
#include "io.h"

#define PIC1 0x20
#define PIC2 0xA0

void init_pic(void) {
	outb(PIC1,     0x11);  // starts the initialization sequence (in cascade mode)
	outb(PIC2,     0x11);
	outb(PIC1 + 1, 0x20);  // ICW2: Master PIC vector offset
	outb(PIC2 + 1, 0x28);  // ICW2: Slave PIC vector offset
	outb(PIC1 + 1, 4);     // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	outb(PIC2 + 1, 2);     // ICW3: tell Slave PIC its cascade identity (0000 0010)
 
	outb(PIC1 + 1, 1);
	outb(PIC2 + 1, 1);

    // Unmask all IRQs, found some in the wild as masked.
    // Initially we only unmasked IRQ0 to get the userspace running, but by now
    // the default userspace programs rely on other IRQs as well (keyboard,
    // uart), so let's just unmask all of them.
    outb(PIC1 + 1, 0);
    outb(PIC2 + 1, 0);
}

void pic_set_handled(int interrupt) {
    interrupt -= 0x20;

    outb(PIC1, 0x20);
    if(interrupt > 7) {
        outb(PIC2, 0x20);
    }
}
