set(image_size_hd 256 CACHE STRING "hd.img size")

math(EXPR image_size_hd_sectors "${image_size_hd} * 1024 * 1024 / 512")
math(EXPR boot_fs_sectors "${image_size_hd_sectors} - 2048")

add_custom_target(hd.img
    COMMAND dd if=/dev/zero of=hd.img bs=512 count=${image_size_hd_sectors}
    COMMAND /sbin/sgdisk -Z -o -n 1:2048:${image_size_headers} -t 1:ef00 hd.img
    COMMAND mformat -i hd.img@@1M -F -L ${boot_fs_sectors}
    COMMAND mcopy   -i hd.img@@1M -sbnmv ${CMAKE_BINARY_DIR}/shared/* ::/
)
