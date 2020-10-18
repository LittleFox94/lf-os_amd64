find_file(ovmf_firmware OVMF.fd HINTS /usr/share/ovmf /usr/share/qemu)

add_custom_target(qemu
    COMMAND qemu-system-x86_64 -bios ${ovmf_firmware} -m 512 -hda hd.img -serial stdio -s -S
    DEPENDS hd.img
    USES_TERMINAL
)

add_custom_target(doc
    COMMAND doxygen ${CMAKE_BINARY_DIR}/Doxyfile
    DEPENDS Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

file(WRITE ${CMAKE_BINARY_DIR}/Doxyfile
    "@INCLUDE = ${CMAKE_SOURCE_DIR}/Doxyfile\n"
    "OUTPUT_DIRECTORY = ${CMAKE_BINARY_DIR}/doc"
)
