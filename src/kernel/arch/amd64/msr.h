#ifndef _MSR_H_INCLUDED
#define _MSR_H_INCLUDED

#include <stdint.h>

void     write_msr(uint32_t msr, uint64_t value);
uint64_t read_msr(uint32_t msr);

#endif
