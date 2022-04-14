#ifndef _EFI_H_INCLUDED
#define _EFI_H_INCLUDED

#include <loader.h>

#define EFIABI __attribute__((ms_abi))
#include <efi/efi.h>

void init_efi(struct LoaderStruct* loaderStruct);
void efi_append_log(char* msg);

#endif
