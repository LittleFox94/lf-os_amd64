SGDISK    := /sbin/sgdisk
MKVFAT    := /sbin/mkfs.vfat
QEMUFLAGS := -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=hd.img -monitor stdio -m 2G -d int --accel tcg

all: test-kvm

test: runnable-image
	qemu-system-x86_64 $(QEMUFLAGS) -s

test-kvm: runnable-image
	kvm $(QEMUFLAGS)

debug: runnable-image src/kernel/arch/amd64-uefi/main.efi
	qemu-system-x86_64 $(QEMUFLAGS) -s -S

bootfs.img:
	dd if=/dev/zero of=bootfs.img bs=1k count=65536
	$(MKVFAT) bootfs.img -F 32
	mmd -i bootfs.img ::/LFOS
	mmd -i bootfs.img ::/EFI
	mmd -i bootfs.img ::/EFI/BOOT

bootable-filesystem: bootfs.img src/kernel/arch/amd64-uefi/main.efi src/init/init
	mcopy -i bootfs.img -o src/kernel/arch/amd64-uefi/main.efi ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i bootfs.img -o src/init/init  ::/LFOS/init

hd.img:
	dd if=/dev/zero of=hd.img bs=512 count=133156
	$(SGDISK) -Z hd.img
	$(SGDISK) -o hd.img
	$(SGDISK) -n 1:2048:133120 -t 1:ef00 hd.img

runnable-image: hd.img bootable-filesystem
	dd if=bootfs.img of=hd.img seek=2048 obs=512 ibs=512 count=131072 conv=notrunc

hd.img.gz: runnable-image
	gzip -fk hd.img

src/kernel/arch/amd64-uefi/main.efi:
	+ make -C src/kernel/arch/amd64-uefi

src/init/init:
	+ make -C src/init

install: src/kernel/arch/amd64-uefi/main.efi src/init/init
	cp src/kernel/arch/amd64-uefi/main.efi /boot/efi/EFI/LFOS/bootx64.efi
	cp src/init/init /boot/efi/LFOS/init

clean:
	+ make -C src/kernel/arch/amd64-uefi clean
	+ make -C src/init clean
	rm -f bootfs.img hd.img hd.img.gz

.PHONY: clean all test test-kvm src/kernel/arch/amd64-uefi/main.efi src/init/init
