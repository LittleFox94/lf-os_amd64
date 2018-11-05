#ifndef _STRING_H_INCLUDED
#define _STRING_H_INCLUDED

#include "stdarg.h"

int kvsnprintf(char* buffer, int buffer_size, const char* format, va_list args);

#endif
