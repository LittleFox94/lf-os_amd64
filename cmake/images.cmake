set(image_size_hd 256 CACHE STRING "hd.img size")
find_program(dd      NAMES dd      DOC "dd program to create disk images")
find_program(sgdisk  NAMES sgdisk  DOC "sgdisk program for manipulating GPT on disk images")
find_program(mformat NAMES mformat DOC "mformat program for creating MS-DOS file systems on disk images")
find_program(mcopy   NAMES mcopy   DOC "mcopy program for copying files on MS-DOS formated disk images")

math(EXPR image_size_hd_sectors "${image_size_hd} * 1024 * 1024 / 512")
math(EXPR boot_fs_sectors "${image_size_hd_sectors} - 2048")

add_custom_target(hd.img
    COMMAND ${dd} if=/dev/zero of=hd.img bs=512 count=${image_size_hd_sectors}
    COMMAND ${sgdisk}  -Z -o -n 1:2048:${image_size_headers} -t 1:ef00 hd.img
    COMMAND ${mformat} -i hd.img@@1M -F -L ${boot_fs_sectors}
    COMMAND ${mcopy}   -i hd.img@@1M -sbnmv ${CMAKE_BINARY_DIR}/shared/* ::/
)
