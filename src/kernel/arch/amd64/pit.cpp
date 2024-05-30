#include "pit.h"
#include "io.h"

void init_pit(void) {
    //outb(0x43, 0x34);
    outb(0x43, 0x30); // disable for HPET instead
}
