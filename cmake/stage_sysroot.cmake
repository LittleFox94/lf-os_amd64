include(helper_targets)

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/syscalls.h
    COMMAND src/syscall-generator.pl src/syscalls.yml ${PROJECT_BINARY_DIR}/syscalls.h user
    DEPENDS
        src/syscalls.yml
        src/syscall-generator.pl
    COMMENT           "Generating userspace syscall bindings"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_target(generate_syscalls.h ALL DEPENDS ${PROJECT_BINARY_DIR}/syscalls.h)

install(
    FILES
        src/include/sys/errno-defs.h
        src/include/sys/known_services.h
        src/include/sys/message_passing.h
        src/include/arch/${architecture}/io.h
        ${PROJECT_BINARY_DIR}/syscalls.h
    DESTINATION
        ${lf_os_sysroot}/include/sys/
)
install(
    FILES
        src/include/uuid.h
    DESTINATION
        ${lf_os_sysroot}/include/
)
