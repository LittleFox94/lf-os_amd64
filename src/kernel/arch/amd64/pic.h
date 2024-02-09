#ifndef _PIC_H_INCLUDED
#define _PIC_H_INCLUDED

// XXX: write APIC driver instead

void init_pic(void);
void pic_set_handled(int interrupt);

#endif
