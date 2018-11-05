CFLAGS := -std=gnu11 -fpic -fno-stack-protector -fshort-wchar -mno-red-zone
SGDISK := /sbin/sgdisk
MKVFAT := /sbin/mkfs.vfat

all: test-kvm

clean:
	rm main.o || true
	rm mm.o || true
	rm main.so || true
	rm main.efi || true
	rm bootfs.img || true
	rm hd.img || true
	rm hd.img.gz || true

test: runnable-image
	qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=hd.img -monitor stdio -m 8G

test-kvm: runnable-image
	kvm -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=hd.img -monitor stdio -m 8G

main.o: main.c
	gcc main.c -c $(CFLAGS) -I /usr/include/efi/ -I /usr/include/efi/x86_64/ -DEFI_FUNCTION_WRAPPER -o main.o

mm.o: mm.c mm.h
	gcc mm.c -c $(CFLAGS) -o mm.o

main.so: main.o mm.o
	ld main.o mm.o /usr/lib/crt0-efi-x86_64.o -nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds -shared -Bsymbolic -L /usr/lib/ -lgnuefi -lefi -o main.so

main.efi: main.so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 main.so main.efi

bootfs.img: main.efi
	dd if=/dev/zero of=bootfs.img bs=1k count=65536
	$(MKVFAT) bootfs.img -F 32
	mmd -i bootfs.img ::/LFOS
	mmd -i bootfs.img ::/EFI
	mmd -i bootfs.img ::/EFI/BOOT

bootable-fs: bootfs.img main.efi
	mcopy -i bootfs.img -o main.efi ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i bootfs.img -o main.efi ::/LFOS/init

hd.img:
	dd if=/dev/zero of=hd.img bs=512 count=133156
	$(SGDISK) -Z hd.img
	$(SGDISK) -o hd.img
	$(SGDISK) -n 1:2048:133120 -t 1:ef00 hd.img

runnable-image: hd.img bootable-fs
	dd if=bootfs.img of=hd.img seek=2048 obs=512 ibs=512 count=131072 conv=notrunc

compressed-image: runnable-image
	gzip -fk hd.img

.PHONY: all clean test test-kvm
