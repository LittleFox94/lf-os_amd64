#include "pit.h"
#include "io.h"

void init_pit(void) {
    int counter = 1193182 / 100;
    outb(0x43, 0x34);
    outb(0x40,counter & 0xFF);
    outb(0x40,counter >> 8);
}
