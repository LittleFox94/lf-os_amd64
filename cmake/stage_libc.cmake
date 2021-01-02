set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib")

include(ExternalProject)
ExternalProject_Add(
    newlib
    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/src/newlib
    INSTALL_DIR ${CMAKE_BINARY_DIR}/install
    CONFIGURE_COMMAND
        ${CMAKE_SOURCE_DIR}/src/newlib/configure
        --target=x86_64-pc-lf_os
        --with-build-sysroot=${toolchain}
        --with-build-time-tools=${toolchain}
        --prefix=${CMAKE_BINARY_DIR}/install
        CC_FOR_TARGET=${toolchain}/bin/clang
        AS_FOR_TARGET=${toolchain}/bin/clang
        LD_FOR_TARGET=${toolchain}/bin/ld.lld
        CXX_FOR_TARGET=${toolchain}/bin/clang
        AR_FOR_TARGET=${toolchain}/bin/llvm-ar
        RANLIB_FOR_TARGET=${toolchain}/bin/llvm-ranlib
        READELF_FOR_TARGET=${toolchain}/bin/llvm-readelf
        OBJCOPY_FOR_TARGET=${toolchain}/bin/llvm-objcopy
        OBJDUMP_FOR_TARGET=${toolchain}/bin/llvm-objdump
        STRIP_FOR_TARGET=${toolchain}/bin/llvm-strip
        "CFLAGS_FOR_TARGET=${CMAKE_C_FLAGS} -flto"
        "LDFLAGS_FOR_TARGET=${CMAKE_LINKER_FLAGS}"
    BUILD_ALWAYS ON
    STEP_TARGETS install
)

add_subdirectory(src/userspace/libpthread)
add_dependencies(pthread newlib-install)
target_include_directories(pthread PRIVATE ${CMAKE_BINARY_DIR}/install/x86_64-pc-lf_os/include)

install(DIRECTORY ${CMAKE_BINARY_DIR}/install/x86_64-pc-lf_os/ DESTINATION ${toolchain})
