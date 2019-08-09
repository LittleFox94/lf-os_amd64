GNUEFI_INCLUDE_DIR := /usr/include/efi
CFLAGS             := -O2 -std=gnu11 -fpic -fomit-frame-pointer -fno-stack-protector -fshort-wchar -mno-red-zone -Wall -fno-builtin -ffreestanding -nostdinc -I $(GNUEFI_INCLUDE_DIR) -I $(GNUEFI_INCLUDE_DIR)/x86_64 -I . -Werror

OBJECTS = loader.o
SOURCES = $(OBJECTS:.o=.c)

all: loader.efi loader.efi.debug

clean:
	rm -f $(OBJECTS)
	rm -f loader.so
	rm -f loader.efi
	rm -f loader.efi.debug

%.o: %.c config.h
	gcc $< -c $(CFLAGS) -DEFI_FUNCTION_WRAPPER -o $@

loader.so: $(OBJECTS)
	ld $(OBJECTS) /usr/lib/crt0-efi-x86_64.o -nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds -shared --no-undefined -Bsymbolic -L /usr/lib/ -lgnuefi -lefi -o loader.so

loader.efi: loader.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 loader.so loader.efi

loader.efi.debug: loader.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc -j .debug_info -j .debug_abbrev -j .debug_loc -j .debug_arranges -j .debug_line -j .debug_macinfo -j .debug_str --target=efi-app-x86_64 loader.so loader.efi.debug

.PHONY: all clean version.c
