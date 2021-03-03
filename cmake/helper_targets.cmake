find_file(ovmf_firmware OVMF.fd HINTS /usr/share/ovmf /usr/share/qemu DOC "OVMF UEFI firmware for QEMU")
find_program(qemu NAMES qemu-system-x86_64 DOC "QEMU for x86_64 emulation")
find_program(kvm  NAMES kvm                DOC "QEMU for x86_64 emulation with KVM acceleration")
find_program(gdb  NAMES gdb                DOC "GNU debugger")

set(QEMU_MEMORY 512M)
set(QEMU_FLAGS_COMMON -m ${QEMU_MEMORY} -d int,guest_errors --serial file:log.txt -s)
set(QEMU_FLAGS_UEFI ${QEMU_FLAGS_COMMON} -drive format=raw,file=hd.img,if=none,id=boot_drive -device nvme,drive=boot_drive,serial=1234 -bios ${ovmf_firmware})
set(QEMU_FLAGS_BIOS ${QEMU_FLAGS_COMMON} -drive format=raw,file=hd.img,if=none,id=boot_drive -device ide-hd,drive=boot_drive,serial=1234)

set(qemu_boot_mode "uefi" CACHE STRING "Boot with uefi or bios?")

if(qemu_boot_mode STREQUAL "uefi")
    set(QEMU_FLAGS ${QEMU_FLAGS_UEFI})
elseif(qemu_boot_mode STREQUAL "bios")
    set(QEMU_FLAGS ${QEMU_FLAGS_BIOS})
endif()

add_custom_target(debug
    COMMAND ${qemu} ${QEMU_FLAGS} --daemonize -S
    COMMAND ${gdb} -ix ${CMAKE_SOURCE_DIR}/.gdbinit
    DEPENDS hd.img .gdbinit
    USES_TERMINAL
)

add_custom_target(debug-kvm
    COMMAND ${kvm} ${QEMU_FLAGS} --daemonize -S
    COMMAND ${gdb} -ix ${CMAKE_SOURCE_DIR}/.gdbinit
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
