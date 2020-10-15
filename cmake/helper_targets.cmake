find_file(ovmf_firmware OVMF.fd HINTS /usr/share/ovmf /usr/share/qemu)

add_custom_target(qemu
    COMMAND qemu-system-x86_64 -bios ${ovmf_firmware} -m 512 -hda hd.img -serial stdio -s -S
    DEPENDS hd.img
    USES_TERMINAL
)
