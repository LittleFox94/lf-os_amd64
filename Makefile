SGDISK    		   := /sbin/sgdisk
MKVFAT    		   := /sbin/mkfs.vfat

QEMU_MEMORY        := 512M
QEMUFLAGS 		   := -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=hd.img,if=none,id=boot_drive -device nvme,drive=boot_drive,serial=1234 -m $(QEMU_MEMORY) -d int,guest_errors --serial file:log.txt
QEMUFLAGS_NO_DEBUG := -monitor stdio

default: run-kvm

doc: src/kernel/arch/amd64/kernel # to get some generated sources
	doxygen Doxyfile

test:
	+ make -C t/kernel

run: hd.img
	qemu-system-x86_64 $(QEMUFLAGS) $(QEMUFLAGS_NO_DEBUG) -s

run-kvm: hd.img
	kvm $(QEMUFLAGS) $(QEMUFLAGS_NO_DEBUG)

profile: hd.img util/gsp/gsp-trace util/gsp/gsp-syms
	gdb --args ./util/gsp/gsp-trace -c -i src/kernel/arch/amd64/kernel -o lf-os.trace -g "stdio:kvm $(QEMUFLAGS) -gdb stdio -S -display vnc=:0 -monitor telnet:localhost:5555,server,nowait" -S sc_handle_scheduler_exit

debug: hd.img src/kernel/arch/amd64/kernel
	gdb -ex "target remote | qemu-system-x86_64 $(QEMUFLAGS) -gdb stdio -S"

debug-kvm: hd.img src/kernel/arch/amd64/kernel
	gdb -ex "target remote | kvm $(QEMUFLAGS) -gdb stdio -S"

lf-os.iso: bootfs.img
	genisoimage --joliet-long -JR --no-emul-boot --eltorito-boot bootfs.img -o $@ bootfs.img

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

lf-os.deb: packaging/control.tar.xz packaging/data.tar.xz
	echo "2.0" > debian-binary
	rm -f lf-os.deb
	ar r lf-os.deb debian-binary packaging/control.tar.xz packaging/data.tar.xz

packaging/control.tar.xz: packaging/debian_*
	tar -cJf $@ packaging/debian_* --transform s.packaging/debian_..

packaging/data.tar.xz: src/loader/loader.efi src/kernel/arch/amd64/kernel src/init/init util/osprobe
	mkdir -p packaging/root/boot/efi/EFI/LFOS/
	mkdir -p packaging/root/boot/efi/LFOS
	mkdir -p packaging/root/usr/lib/os-probes/mounted/efi
	cp src/loader/loader.efi 		packaging/root/boot/efi/EFI/LFOS/BOOTX64.EFI
	cp src/kernel/arch/amd64/kernel packaging/root/boot/efi/LFOS/kernel
	cp src/init/init 	 			packaging/root/boot/efi/LFOS/init
	cp util/osprobe 	 			packaging/root/usr/lib/os-probes/mounted/efi/20lfos
	tar --owner root --group root -cJf $@ packaging/root/* --transform s.packaging/root/..
