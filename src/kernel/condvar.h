#ifndef _CONDVAR_H_INCLUDED
#define _CONDVAR_H_INCLUDED

#include <stdint.h>

typedef uint64_t condvar_t;

void init_condvar(void);

#endif
