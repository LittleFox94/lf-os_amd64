set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib")

include(ExternalProject)
ExternalProject_Add(
    newlib
    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/src/newlib
    INSTALL_DIR ${CMAKE_BINARY_DIR}/install
    CONFIGURE_COMMAND
        "${CMAKE_SOURCE_DIR}/src/newlib/configure"
        "--target=x86_64-pc-lf_os"
        "--with-build-sysroot=${lf_os_sysroot}"
        "--with-build-time-tools=${lf_os_toolchain}"
        "--prefix=${CMAKE_BINARY_DIR}/install"
        "CC_FOR_TARGET=${lf_os_toolchain}/bin/clang"
        "AS_FOR_TARGET=${lf_os_toolchain}/bin/clang"
        "LD_FOR_TARGET=${lf_os_toolchain}/bin/ld.lld"
        "CXX_FOR_TARGET=${lf_os_toolchain}/bin/clang"
        "AR_FOR_TARGET=${lf_os_toolchain}/bin/ar"
        "RANLIB_FOR_TARGET=${lf_os_toolchain}/bin/ranlib"
        "READELF_FOR_TARGET=${lf_os_toolchain}/bin/readelf"
        "OBJCOPY_FOR_TARGET=${lf_os_toolchain}/bin/objcopy"
        "OBJDUMP_FOR_TARGET=${lf_os_toolchain}/bin/objdump"
        "STRIP_FOR_TARGET=${lf_os_toolchain}/bin/strip"
        "CFLAGS_FOR_TARGET=${CMAKE_C_FLAGS}"
        "LDFLAGS_FOR_TARGET=${CMAKE_LINKER_FLAGS}"
    BUILD_ALWAYS ON
    STEP_TARGETS install
)

add_subdirectory(src/userspace/libpthread)
add_dependencies(pthread newlib)
target_include_directories(pthread PRIVATE ${CMAKE_BINARY_DIR}/install/x86_64-pc-lf_os/include)

install(DIRECTORY ${CMAKE_BINARY_DIR}/install/x86_64-pc-lf_os/ DESTINATION ${lf_os_sysroot})
