set(package_userspace ${build_userspace})
list(TRANSFORM package_userspace PREPEND ${CMAKE_INSTALL_PREFIX}/userspace/)

set(initramfs_dest_dir "${CMAKE_BINARY_DIR}/images/initramfs.contents")
file(REMOVE_RECURSE ${initramfs_dest_dir})

foreach(target IN LISTS package_userspace)
    file(COPY ${target} DESTINATION ${initramfs_dest_dir}/)
endforeach()

set(9ptar_fs_sources ${initramfs_dest_dir} CACHE STRING "" FORCE)

add_subdirectory(src/userspace/lib9p)
set_target_properties(server PROPERTIES EXCLUDE_FROM_ALL false)

install(FILES $<TARGET_FILE:server> DESTINATION "initramfs" RENAME initramfs)
