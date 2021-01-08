#ifndef _STRING_H_INCLUDED
#define _STRING_H_INCLUDED

#include <efi.h>

void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);

size_t wcslen(const CHAR16* s);
int wcscmp(const CHAR16* s1, const CHAR16* s2);
size_t wcstombs(char* dest, const CHAR16* src, size_t n);

void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void  free(void* ptr);

int wprintf(const CHAR16* format, ...);

void init_stdlib(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table);

extern EFI_BOOT_SERVICES* BS;

#endif
