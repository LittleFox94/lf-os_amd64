CFLAGS    := $(OPTIMIZATION) -g -std=gnu11 -fno-stack-protector -mno-red-zone -Wall -fno-builtin -ffreestanding -nostdinc -I . -Werror
NASMFLAGS := -g -f elf64

OBJECTS = start.o main.o mm.o fbconsole.o string.o sd.o elf.o sc.o msr.o gdt.O init.O vm.o version.o scheduler.o pic.o pit.o
SOURCES = $(OBJECTS:.o=.c)

all: kernel

clean:
	rm -f $(OBJECTS)
	rm -f kernel.elf
	rm -f kernel

version.c:
	rm -f version.o

%.O: %.asm
	nasm $(NASMFLAGS) -o $@ $<

%.o: %.c config.h
	clang $< -c $(CFLAGS) -o $@

kernel.elf: version.o $(OBJECTS)
	ld.lld $(OBJECTS) -nostdlib -o $@

kernel: kernel.elf
	objcopy $< -O binary -j .text -j .rodata -j .data -j .bss $@

.PHONY: all clean version.c
