include(helper_targets)

install(
    FILES
        src/include/errno.h
        src/include/message_passing.h
        src/include/arch/${architecture}/io.h
        ${CMAKE_CURRENT_BINARY_DIR}/syscalls.h
    DESTINATION
        ${lf_os_sysroot}/include/sys/
)
install(
    FILES
        src/include/uuid.h
    DESTINATION
        ${lf_os_sysroot}/include/
)
