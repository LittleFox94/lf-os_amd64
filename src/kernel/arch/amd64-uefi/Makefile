CFLAGS := -g -std=gnu11 -fpic -fno-stack-protector -fshort-wchar -mno-red-zone -Wall -Werror -fno-builtin
SGDISK := /sbin/sgdisk
MKVFAT := /sbin/mkfs.vfat

OBJECTS = main.o mm.o fbconsole.o string.o
SOURCES = $(OBJECTS:.o=.c)

all: test-kvm

clean:
	rm -f $(OBJECTS)
	rm -f main.so
	rm -f main.efi
	rm -f main.efi.debug
	rm -f bootfs.img
	rm -f hd.img
	rm -f hd.img.gz

test: runnable-image
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=hd.img -monitor stdio -m 8G -s

test-kvm: runnable-image
	kvm -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=hd.img -monitor stdio -m 8G

debug: runnable-image main.efi.debug
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=hd.img -monitor stdio -m 8G -s -S

%.o: %.c
	gcc $< -c $(CFLAGS) -I /usr/include/efi/ -I /usr/include/efi/x86_64/ -DEFI_FUNCTION_WRAPPER -o $@

main.so: $(OBJECTS)
	ld $(OBJECTS) /usr/lib/crt0-efi-x86_64.o -nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds -shared -Bsymbolic -L /usr/lib/ -lgnuefi -lefi -o main.so

main.efi: main.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 main.so main.efi

main.efi.debug: main.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc -j .debug_info -j .debug_abbrev -j .debug_loc -j .debug_arranges -j .debug_line -j .debug_macinfo -j .debug_str --target=efi-app-x86_64 main.so main.efi.debug

bootfs.img: main.efi
	dd if=/dev/zero of=bootfs.img bs=1k count=65536
	$(MKVFAT) bootfs.img -F 32
	mmd -i bootfs.img ::/LFOS
	mmd -i bootfs.img ::/EFI
	mmd -i bootfs.img ::/EFI/BOOT

bootable-filesystem: bootfs.img
	mcopy -i bootfs.img -o main.efi ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i bootfs.img -o main.efi ::/LFOS/init

hd.img:
	dd if=/dev/zero of=hd.img bs=512 count=133156
	$(SGDISK) -Z hd.img
	$(SGDISK) -o hd.img
	$(SGDISK) -n 1:2048:133120 -t 1:ef00 hd.img

runnable-image: hd.img bootable-filesystem
	dd if=bootfs.img of=hd.img seek=2048 obs=512 ibs=512 count=131072 conv=notrunc

compressed-image: runnable-image
	gzip -fk hd.img

.PHONY: all clean test test-kvm
