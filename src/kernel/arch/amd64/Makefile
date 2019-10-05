CC 	   := clang
CFLAGS := $(OPTIMIZATION) -g -std=gnu11 -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -fno-stack-protector -mno-red-zone -Wall -fno-builtin -ffreestanding -nostdinc -I$(shell pwd) -I$(shell pwd)/../../ -Werror -mcmodel=large

export CC
export CFLAGS

OBJECTS = start.o main.o mm.o sc.o msr.o gdt.o init.o vm.o scheduler.o pic.o pit.o bluescreen.o syscalls.gen.o

kernel: $(OBJECTS) ../../liblfos-kernel.a
	ld.lld -o $@ $(OBJECTS) -L ../../ -llfos-kernel --image-base 0xFFFF800001000000

../../liblfos-kernel.a:
	+ make -C ../../

%.o: %.c ../../config.h
	$(CC) $< -c $(CFLAGS) -o $@

syscalls.gen.c: ../../../syscalls.yml ../../../syscall-generator.pl
	../../../syscall-generator.pl ../../../syscalls.yml $@ kernel

%.o: %.S ../../config.h
	$(CC) $< -c $(CFLAGS) -o $@

clean:
	rm -f $(OBJECTS)
	rm -f kernel.elf
	rm -f kernel

.PHONY: ../../liblfos-kernel.a
