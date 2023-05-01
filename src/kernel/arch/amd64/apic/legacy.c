#include <io.h>

void init_legacy_pit() {
    outb(0x43, 0x00);
}

void init_legacy_pic() {
    init_legacy_pit();

    const uint16_t PIC1 = 0x20;
    const uint16_t PIC2 = 0xA0;

    outb(PIC1,     0x11);  // starts the initialization sequence (in cascade mode)
    outb(PIC2,     0x11);
    outb(PIC1 + 1, 0xF0);  // ICW2: Master PIC vector offset
    outb(PIC2 + 1, 0xF8);  // ICW2: Slave PIC vector offset
    outb(PIC1 + 1, 4);     // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC2 + 1, 2);     // ICW3: tell Slave PIC its cascade identity (0000 0010)

    outb(PIC1 + 1, 1);
    outb(PIC2 + 1, 1);

    // mask everything as we only want to use the APIC
    uint8_t mask = 0xFF;
    outb(PIC1 + 1, mask);
    outb(PIC2 + 1, mask);
}
