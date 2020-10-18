find_file(ovmf_firmware OVMF.fd HINTS /usr/share/ovmf /usr/share/qemu DOC "OVMF UEFI firmware for QEMU")
find_program(qemu NAMES qemu-system-x86_64 DOC "QEMU for x86_64 emulation")
find_program(kvm  NAMES kvm                DOC "QEMU for x86_64 emulation with KVM acceleration")
find_program(gdb  NAMES gdb                DOC "GNU debugger")

set(QEMU_MEMORY 512M)
set(QEMU_FLAGS -bios ${ovmf_firmware} -drive format=raw,file=hd.img,if=none,id=boot_drive -device nvme,drive=boot_drive,serial=1234 -m ${QEMU_MEMORY} -d int,guest_errors --serial file:log.txt -s)

add_custom_target(debug
    COMMAND ${qemu} ${QEMU_FLAGS} --daemonize -S
    COMMAND ${gdb}
    DEPENDS hd.img .gdbinit
    USES_TERMINAL
)

add_custom_target(debug-kvm
    COMMAND ${kvm} ${QEMU_FLAGS} --daemonize -S
    COMMAND ${gdb}
    DEPENDS hd.img .gdbinit
    USES_TERMINAL
)

add_custom_target(run
    COMMAND ${qemu} ${QEMU_FLAGS}
    DEPENDS hd.img
    USES_TERMINAL
)

add_custom_target(run-kvm
    COMMAND ${kvm} ${QEMU_FLAGS}
    DEPENDS hd.img
    USES_TERMINAL
)

add_custom_target(doc
    COMMAND doxygen ${CMAKE_BINARY_DIR}/Doxyfile
    DEPENDS Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

configure_file(Doxyfile.in Doxyfile)
