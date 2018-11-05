CFLAGS := -g -std=gnu11 -fpic -fno-stack-protector -fshort-wchar -mno-red-zone -Wall -fno-builtin -ffreestanding -nostdinc -I . -march=native -mtune=native

OBJECTS = main.o mm.o fbconsole.o string.o sd.o elf.o
SOURCES = $(OBJECTS:.o=.c)

all: main.efi 

clean:
	rm -f $(OBJECTS)
	rm -f main.so
	rm -f main.efi
	rm -f main.efi.debug

%.o: %.c
	gcc $< -c $(CFLAGS) -I /usr/include/efi/ -I /usr/include/efi/x86_64/ -DEFI_FUNCTION_WRAPPER -o $@

main.so: $(OBJECTS)
	ld $(OBJECTS) /usr/lib/crt0-efi-x86_64.o -nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds -shared --no-undefined -Bsymbolic -L /usr/lib/ -lgnuefi -lefi -o main.so

main.efi: main.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 main.so main.efi

main.efi.debug: main.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc -j .debug_info -j .debug_abbrev -j .debug_loc -j .debug_arranges -j .debug_line -j .debug_macinfo -j .debug_str --target=efi-app-x86_64 main.so main.efi.debug

install: main.efi
	cp main.efi /boot/efi/EFI/LFOS/bootx64.efi
	cp main.efi /boot/efi/LFOS/init

.PHONY: all clean test test-kvm
