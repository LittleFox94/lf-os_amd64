#ifndef _STRING_H_INCLUDED
#define _STRING_H_INCLUDED

#include "stdarg.h"
#include "stdint.h"

void  memset32(uint32_t* dest, uint32_t c, size_t size);
void* memset(void* dest, int c, size_t size);
void* memcpy(void* dest, void const* source, size_t size);
size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
char* strncpy(char* s1, const char* s2, size_t n);

int ksnprintf(char* buffer, int buffer_size, const char* format, ...);
int kvsnprintf(char* buffer, int buffer_size, const char* format, va_list args);

int sputs(char* buffer, int buffer_size, char* string, int length);
int sputui(char* buffer, int buffer_size, uint64_t number, int base);
int sputi(char* buffer, int buffer_size, int64_t number, int base);
int sputbytes(char* buffer, int buffer_size, int64_t number);

#endif
