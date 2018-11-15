CFLAGS    := -g -std=gnu11 -fpic -fno-stack-protector -mno-red-zone -Wall -fno-builtin -ffreestanding -nostdinc -I . -Werror
NASMFLAGS := -g -f elf64

OBJECTS = main.o mm.o fbconsole.o string.o sd.o elf.o sc.o msr.o gdt.O init.O vm.o version.o scheduler.o pic.o pit.o
SOURCES = $(OBJECTS:.o=.c)

all: kernel

clean:
	rm -f $(OBJECTS)
	rm -f kernel

version.c:
	rm -f version.o

%.O: %.asm
	nasm $(NASMFLAGS) -o $@ $<

%.o: %.c config.h
	gcc $< -c $(CFLAGS) -I /usr/include/efi/ -I /usr/include/efi/x86_64/ -DEFI_FUNCTION_WRAPPER -o $@

kernel: version.o $(OBJECTS)
	ld $(OBJECTS) -nostdlib -o $@

.PHONY: all clean version.c
