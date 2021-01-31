find_program(tar NAMES tar DOC "tar archive program - must generate ustar format")

set(9ptar_fs_sources ${PROJECT_SOURCE_DIR}            CACHE STRING "Root directory to archive for 9ptar")
set(9ptar_fs_exclude --exclude=build --exclude=.*.s?? CACHE STRING "tar --exclude directives")

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/tar_archive
    COMMAND ${tar} cf ${PROJECT_BINARY_DIR}/tar_archive -C ${9ptar_fs_sources} ${9ptar_fs_exclude} .
)

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/tar.o
    DEPENDS ${PROJECT_BINARY_DIR}/tar_archive
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND ${CMAKE_OBJCOPY} -I binary -O elf64-x86-64 tar_archive tar.o
)

add_executable(server
    main.cpp
    ${PROJECT_BINARY_DIR}/tar.o
)

target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR}/lib)
target_link_libraries(server 9p)
set_target_properties(server PROPERTIES EXCLUDE_FROM_ALL true)
