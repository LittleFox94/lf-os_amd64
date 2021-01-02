profile: hd.img util/gsp/gsp-trace util/gsp/gsp-syms
	gdb --args ./util/gsp/gsp-trace -c -i src/kernel/arch/amd64/kernel -o lf-os.trace -g "stdio:kvm $(QEMUFLAGS) -gdb stdio -S -display vnc=:0 -monitor telnet:localhost:5555,server,nowait" -S sc_handle_scheduler_exit

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
