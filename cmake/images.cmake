set(image_size_hd 256 CACHE STRING "hd.img size")
find_program(xz      NAMES xz      DOC "xz compression tool")
find_program(dd      NAMES dd      DOC "dd program to create disk images")
find_program(sgdisk  NAMES sgdisk  DOC "sgdisk program for manipulating GPT on disk images")

math(EXPR image_size_hd_sectors "${image_size_hd} * 1024 * 1024 / 512")
# Partition starts at sector 2048 (1MiB) and leaves enough space for backup GPT at the end
math(EXPR boot_fs_sectors "${image_size_hd_sectors} - 2048 - 33")

function(make_bootable_image destination)
    set(dest_dir "${CMAKE_BINARY_DIR}/images/${destination}.contents")

    set(targets ${ARGN})

    list(TRANSFORM targets PREPEND "${CMAKE_BINARY_DIR}/")
    list(JOIN targets ";" targets_string)

    add_custom_target(${destination}
        COMMAND ${CMAKE_COMMAND}
            -Ddest_dir=${dest_dir}
            -Dkernel=${CMAKE_BINARY_DIR}/shared/kernel/kernel
            -Dloader=${CMAKE_BINARY_DIR}/shared/loader/loader.efi
            "-Dtargets=\"${targets_string}\""
            -P ${CMAKE_SOURCE_DIR}/cmake/image_contents.cmake
        COMMAND $<TARGET_FILE:fatcreate>
            -s ${boot_fs_sectors}
            -o ${destination}.fat32
            ${dest_dir}/*
        COMMAND ${dd}
            if=/dev/zero
            of=${destination}
            bs=512
            count=${image_size_hd_sectors}
            conv=sparse
        COMMAND ${sgdisk}
            -Z -o
            -n 1:2048:+${boot_fs_sectors}
            -t 1:ef00
            ${destination}
        COMMAND ${dd}
            if=${destination}.fat32
            of=${destination}
            bs=512
            seek=2048
            conv=notrunc,sparse
    )

    add_custom_target(${destination}.xz
        DEPENDS ${destination}
        COMMAND ${xz} -k ${CMAKE_BINARY_DIR}/${destination}
    )
endfunction()

set(package_userspace ${build_userspace})
list(TRANSFORM package_userspace PREPEND shared/userspace/)
make_bootable_image(
    hd.img
    ${package_userspace}
)

install(FILES ${CMAKE_BINARY_DIR}/shared/loader/loader.efi   DESTINATION boot/efi/EFI/LFOS RENAME BOOTX64.EFI)
install(FILES ${CMAKE_BINARY_DIR}/shared/kernel/kernel       DESTINATION boot/efi/LFOS)

foreach(userspace IN LISTS build_userspace)
    install(FILES ${CMAKE_BINARY_DIR}/shared/userspace/${userspace}  DESTINATION boot/efi/LFOS)
endforeach()

install(PROGRAMS util/osprobe RENAME 20lfos           DESTINATION usr/lib/os-probes/mounted/efi)

set(CPACK_PACKAGE_NAME lf_os)
set(CPACK_PACKAGE_FILE_NAME "lf_os_${CPACK_PACKAGE_VERSION}_${architecture}")
set(CPACK_PACKAGE_CONTACT  "Mara Sophie Grosch <littlefox@lf-net.org>")
set(CPACK_PACKAGE_HOMEPAGE "https://praios.lf-net.org/littlefox/lf-os_amd64")
set(CPACK_PACKAGING_INSTALL_PREFIX "/")

set(CPACK_GENERATOR        "DEB;TXZ")
set(CPACK_SOURCE_GENERATOR "")

set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Bootable LF OS system")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_COMPRESSION_TYPE   xz)
set(CPACK_DEBIAN_PACKAGE_SECTION    kernel)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "grub-efi-amd64, os-prober")

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    ${CMAKE_SOURCE_DIR}/packaging/preinst
    ${CMAKE_SOURCE_DIR}/packaging/postinst
    ${CMAKE_SOURCE_DIR}/packaging/postrm
)

include(CPack)
