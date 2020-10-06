add_custom_target(hd.img
    COMMAND dd if=/dev/zero of=hd.img bs=512 count=133156
    COMMAND /sbin/sgdisk -Z -o -n 1:2048:133120 -t 1:ef00 hd.img
    COMMAND mformat -i hd.img@@1M -F -L 131072
    COMMAND mcopy   -i hd.img@@1M -sbnmv ${CMAKE_BINARY_DIR}/shared/* ::/
)
