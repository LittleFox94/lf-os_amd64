set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

add_subdirectory(src/userspace/lib9p)
install(TARGETS 9p                  DESTINATION ${lf_os_sysroot}/lib)
install(TARGETS 9p FILE_SET headers DESTINATION ${lf_os_sysroot}/include)
