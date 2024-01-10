file(REMOVE_RECURSE ${dest_dir})

file(MAKE_DIRECTORY
    ${dest_dir}/EFI/BOOT
    ${dest_dir}/EFI/LFOS
    ${dest_dir}/LFOS
)

file(COPY_FILE ${loader} ${dest_dir}/EFI/BOOT/BOOTX64.EFI)
file(COPY_FILE ${loader} ${dest_dir}/EFI/LFOS/BOOTX64.EFI)
file(COPY_FILE ${kernel} ${dest_dir}/LFOS/kernel)

foreach(target IN LISTS targets)
    file(COPY ${target} DESTINATION ${dest_dir}/LFOS)
endforeach()
