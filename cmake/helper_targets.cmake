find_file(ovmf_firmware OVMF.fd HINTS /usr/share/ovmf /usr/share/qemu DOC "OVMF UEFI firmware for QEMU")
find_program(qemu_img NAMES qemu-img           DOC "QEMU image management tool")
find_program(qemu     NAMES qemu-system-x86_64 DOC "QEMU for x86_64 emulation")
find_program(kvm      NAMES kvm                DOC "QEMU for x86_64 emulation with KVM acceleration")
find_program(gdb      NAMES gdb                DOC "GNU debugger")

set(QEMU_MEMORY 512M)
set(QEMU_FLAGS      -drive format=qcow2,file=firmware.qcow2,if=pflash,readonly=on -m ${QEMU_MEMORY} -machine q35 -d int,guest_errors --serial file:log.txt -device qemu-xhci --device isa-debugcon,iobase=0x402,chardev=debug -chardev file,id=debug,path=debug.log)
set(QEMU_FLAGS_NVME -drive format=raw,file=hd.img,if=none,id=boot_drive -device nvme,drive=boot_drive,serial=1234)
set(QEMU_FLAGS_PXE  -netdev user,id=net0,tftp=${CMAKE_BINARY_DIR}/shared,bootfile=/EFI/LFOS/BOOTX64.efi -device virtio-net,netdev=net0,romfile=)

add_custom_target(firmware.qcow2
    COMMAND ${qemu_img} create -o backing_file=${ovmf_firmware} -o backing_fmt=raw -o cluster_size=512 -f qcow2 firmware.qcow2
    DEPENDS ${ovmf_firmware}
)

add_custom_target(debug
    COMMAND ${qemu} ${QEMU_FLAGS} ${QEMU_FLAGS_NVME} --daemonize -s -S
    COMMAND ${gdb} -ix gdbinit
    DEPENDS hd.img gdbinit firmware.qcow2
    USES_TERMINAL
)

add_custom_target(debug-pxe
    COMMAND ${qemu} ${QEMU_FLAGS} ${QEMU_FLAGS_PXE} --daemonize -s -S
    COMMAND ${gdb} -ix gdbinit
    DEPENDS hd.img gdbinit firmware.qcow2
    USES_TERMINAL
)

add_custom_target(debug-kvm
    COMMAND ${kvm} ${QEMU_FLAGS} ${QEMU_FLAGS_NVME} --daemonize -s -S
    COMMAND ${gdb} -ix gdbinit
    DEPENDS hd.img gdbinit firmware.qcow2
    USES_TERMINAL
)

add_custom_target(run
    COMMAND ${qemu} ${QEMU_FLAGS} ${QEMU_FLAGS_NVME}
    DEPENDS hd.img firmware.qcow2
    USES_TERMINAL
)

add_custom_target(run-pxe
    COMMAND ${qemu} ${QEMU_FLAGS} ${QEMU_FLAGS_PXE}
    DEPENDS hd.img firmware.qcow2
    USES_TERMINAL
)

add_custom_target(run-kvm
    COMMAND ${kvm} ${QEMU_FLAGS} ${QEMU_FLAGS_NVME}
    DEPENDS hd.img firmware.qcow2
    USES_TERMINAL
)

add_custom_target(
    syscalls.h  ALL
    src/syscall-generator.pl src/syscalls.yml ${PROJECT_BINARY_DIR}/syscalls.h user
    SOURCES
        src/syscalls.yml
        src/syscall-generator.pl
    COMMENT           "Generating userspace syscall bindings"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_target(doc
    COMMAND doxygen ${CMAKE_BINARY_DIR}/Doxyfile
    DEPENDS Doxyfile syscalls.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

configure_file(Doxyfile.in Doxyfile)

set(GDB_COMMANDS build_userspace)
list(TRANSFORM GDB_COMMANDS PREPEND "add-symbol-file shared/userspace/")
configure_file(gdbinit.in gdbinit)
