#ifndef _STRING_H_INCLUDED
#define _STRING_H_INCLUDED

#include "stdarg.h"
#include "stdint.h"

void* memcpy(void* dest, void* source, size_t size);
size_t strlen(char* str);

int kvsnprintf(char* buffer, int buffer_size, const char* format, va_list args);

#endif
