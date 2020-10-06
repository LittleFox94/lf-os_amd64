#ifndef _STRING_H_INCLUDED
#define _STRING_H_INCLUDED

void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);

size_t wcslen(const CHAR16* s);
int wcscmp(const CHAR16* s1, const CHAR16* s2);

void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void  free(void* ptr);

int wprintf(const CHAR16* format, ...);

#include <efi.h>
void init_stdlib(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table);
extern EFI_BOOT_SERVICES* BS;

#endif
