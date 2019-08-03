#ifndef _LFOSTEST_H_INCLUDED
#define _LFOSTEST_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../../src/kernel/stdarg.h"

#include <stdio.h>

typedef uint64_t ptr_t;

#define eq(EXPECTED, ACTUAL, MESSAGE) if(EXPECTED != ACTUAL) error = true; printf("%s: %s\n", MESSAGE, EXPECTED == ACTUAL ? "OK" : "NOK")

#endif
