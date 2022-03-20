set(image_size_hd 256 CACHE STRING "hd.img size")
find_program(xz      NAMES xz      DOC "xz compression tool")
find_program(dd      NAMES dd      DOC "dd program to create disk images")
find_program(sgdisk  NAMES sgdisk  DOC "sgdisk program for manipulating GPT on disk images")

math(EXPR image_size_hd_sectors "${image_size_hd} * 1024 * 1024 / 512")
# Partition starts at sector 2048 (1MiB) and leaves enough space for backup GPT at the end
math(EXPR boot_fs_sectors "${image_size_hd_sectors} - 2048 - 33")

add_custom_target(bootfs.img
    COMMAND $<TARGET_FILE:fatcreate>
        -s ${boot_fs_sectors}
        -o bootfs.img
        ${CMAKE_BINARY_DIR}/shared/*
)

add_custom_target(hd.img
    DEPENDS bootfs.img
    COMMAND ${dd} if=/dev/zero of=hd.img bs=512 count=${image_size_hd_sectors} conv=sparse
    COMMAND ${sgdisk} -Z -o -n 1:2048:+${boot_fs_sectors} -t 1:ef00 hd.img
    COMMAND ${dd} if=bootfs.img of=hd.img bs=512 seek=2048 conv=notrunc,sparse
)

add_custom_target(hd.img.xz
    DEPENDS hd.img
    COMMAND ${xz} -k ${CMAKE_BINARY_DIR}/hd.img
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/shared/EFI/LFOS DESTINATION boot/efi/EFI)
install(DIRECTORY ${CMAKE_BINARY_DIR}/shared/LFOS     DESTINATION boot/efi)
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
