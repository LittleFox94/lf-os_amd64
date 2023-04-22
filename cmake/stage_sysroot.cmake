include(helper_targets)

install(
    FILES
        src/include/sys/errno-defs.h
        src/include/sys/known_services.h
        src/include/sys/message_passing.h
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
