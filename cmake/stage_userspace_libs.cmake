set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdlib")

add_subdirectory(src/userspace/lib9p)
install(TARGETS 9p                  DESTINATION ${lf_os_sysroot}/lib)
install(TARGETS 9p FILE_SET headers DESTINATION ${lf_os_sysroot}/include)
