SGDISK    		   := /sbin/sgdisk
MKVFAT    		   := /sbin/mkfs.vfat

QEMU_MEMORY        := 512M
QEMUFLAGS 		   := -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=hd.img,if=none,id=boot_drive -device nvme,drive=boot_drive,serial=1234 -m $(QEMU_MEMORY) -d int,guest_errors --serial file:log.txt
QEMUFLAGS_NO_DEBUG := -monitor stdio

export OPTIMIZATION := -O3

all: run-kvm

doc: runnable-image # to generate some sources
	doxygen Doxyfile

test:
	+ make -C t/kernel

run: runnable-image
	qemu-system-x86_64 $(QEMUFLAGS) $(QEMUFLAGS_NO_DEBUG) -s

run-kvm: runnable-image
	kvm $(QEMUFLAGS) $(QEMUFLAGS_NO_DEBUG)

profile: runnable-image util/gsp/gsp-trace util/gsp/gsp-syms
	./util/gsp/gsp-trace -i src/kernel/arch/amd64/kernel -c -o lf-os.trace -g "stdio:qemu-system-x86_64 $(QEMUFLAGS) -gdb stdio -S -display vnc=:0" -S _panic

debug: runnable-image src/kernel/arch/amd64/kernel
	gdb -ex "target remote | qemu-system-x86_64 $(QEMUFLAGS) -gdb stdio -S"

debug-kvm: runnable-image src/kernel/arch/amd64/kernel
	gdb -ex "target remote | kvm $(QEMUFLAGS) -gdb stdio -S"

bootfs.img:
	dd if=/dev/zero of=bootfs.img bs=1k count=65536
	$(MKVFAT) bootfs.img -F 32
	mmd -i bootfs.img ::/LFOS
	mmd -i bootfs.img ::/EFI
	mmd -i bootfs.img ::/EFI/BOOT

bootable-filesystem: bootfs.img src/loader/loader.efi src/kernel/arch/amd64/kernel src/init/init
	mcopy -i bootfs.img -o src/loader/loader.efi 		::/EFI/BOOT/BOOTX64.EFI
	mcopy -i bootfs.img -o src/init/init  				::/LFOS/init
	mcopy -i bootfs.img -o src/kernel/arch/amd64/kernel ::/LFOS/kernel

hd.img:
	dd if=/dev/zero of=hd.img bs=512 count=133156
	$(SGDISK) -Z hd.img
	$(SGDISK) -o hd.img
	$(SGDISK) -n 1:2048:133120 -t 1:ef00 hd.img

runnable-image: hd.img bootable-filesystem
	dd if=bootfs.img of=hd.img seek=2048 obs=512 ibs=512 count=131072 conv=notrunc

%.gz: % runnable-image
	gzip -fk $<

lf-os.iso: bootable-filesystem
	genisoimage -JR --no-emul-boot --eltorito-boot bootfs.img -o $@ bootfs.img

src/kernel/arch/amd64/kernel:
	+ make -C src/kernel/arch/amd64

src/loader/loader.efi:
	+ make -C src/loader

src/init/init: sysroot
	+ make -C src/init

util/gsp/gsp-trace:
	+ make -C util/gsp gsp-trace

util/gsp/gsp-syms:
	+ make -C util/gsp gsp-syms

util/embed:
	+ make -C util embed

install: src/loader/loader.efi src/kernel/arch/amd64/kernel src/init/init
	cp src/loader/loader.efi /boot/efi/EFI/LFOS/bootx64.efi
	cp src/kernel/arch/amd64/kernel /boot/efi/LFOS/kernel
	cp src/init/init /boot/efi/LFOS/init

sysroot:
	mkdir -p sysroot
	+ make -C sysroot -f ../src/sysroot/Makefile

sysroot.tar.xz: sysroot
	tar cJf sysroot.tar.xz sysroot --transform s~sysroot~x86_64-lf_os-elf~

clean:
	+ make -C src/kernel clean
	+ make -C src/loader clean
	+ make -C src/init clean
	+ make -C src/sysroot clean
	+ make -C util/gsp clean
	+ make -C util clean
	rm -f bootfs.img hd.img hd.img.gz lf-os.iso lf-os.iso.gz

.PHONY: clean all test test-kvm src/kernel/arch/amd64/kernel src/init/init src/loader/loader.efi util/gsp/gsp-trace util/gsp/gsp-syms util/embed sysroot
