project(initramfs CXX)

find_program(tar NAMES tar DOC "tar archive program - must generate ustar format")

set(package_userspace ${build_userspace})
list(TRANSFORM package_userspace PREPEND ${CMAKE_INSTALL_PREFIX}/userspace/)

set(initramfs_dest_dir "${CMAKE_BINARY_DIR}/images/initramfs.contents")
file(REMOVE_RECURSE ${initramfs_dest_dir})

foreach(target IN LISTS package_userspace)
    file(COPY ${target} DESTINATION ${initramfs_dest_dir}/)
endforeach()

add_custom_command(
    OUTPUT              ${PROJECT_BINARY_DIR}/initramfs.tar
    WORKING_DIRECTORY   ${initramfs_dest_dir}
    COMMAND             ${tar}
        cf
        "${PROJECT_BINARY_DIR}/initramfs.tar"
        .
)

add_custom_command(
    OUTPUT              ${PROJECT_BINARY_DIR}/initramfs.tar.o
    DEPENDS             ${PROJECT_BINARY_DIR}/initramfs.tar
    WORKING_DIRECTORY   ${PROJECT_BINARY_DIR}
    COMMAND             ${CMAKE_OBJCOPY}
        -I binary
        -O elf64-x86-64
        initramfs.tar
        initramfs.tar.o
)

add_executable(initramfs
    main.cpp
    tarfilesystem.cpp   tarfilesystem.h

    ${PROJECT_BINARY_DIR}/initramfs.tar.o
)
target_compile_features(initramfs PRIVATE cxx_std_20)
target_link_libraries(initramfs PRIVATE 9p)

install(FILES $<TARGET_FILE:initramfs> DESTINATION initramfs)
