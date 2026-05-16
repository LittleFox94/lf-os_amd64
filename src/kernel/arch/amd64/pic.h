#ifndef _PIC_H_INCLUDED
#define _PIC_H_INCLUDED

#include <stdint.h>
// XXX: write APIC driver instead

void init_pic(void);
void pic_set_handled(int interrupt);
uint16_t pic_get_isr(void);

#endif
