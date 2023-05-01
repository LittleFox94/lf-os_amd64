#ifndef _HPET_H_INCLUDED
#define _HPET_H_INCLUDED

#include <acpi.h>

void init_hpet(struct acpi_table_header* table);
uint64_t hpet_ns_per_tick();
uint64_t hpet_ticks();

#endif // _HPET_H_INCLUDED
