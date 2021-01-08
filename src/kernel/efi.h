#ifndef _EFI_H_INCLUDED
#define _EFI_H_INCLUDED

#include <loader.h>

void init_efi(LoaderStruct* loaderStruct);
void efi_append_log(char* msg);

#endif
