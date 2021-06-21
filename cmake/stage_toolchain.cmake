cmake_minimum_required(VERSION 3.10)
project(toolchain C CXX ASM)

set(LLVM_ENABLE_PROJECTS "clang;lld" CACHE STRING "" FORCE)
set(LLVM_DEFAULT_TARGET_TRIPLE "x86_64-pc-lf_os" CACHE STRING "" FORCE)
set(LLVM_TARGETS_TO_BUILD "X86" CACHE STRING "" FORCE)
set(LLVM_BUILD_TOOLS ON CACHE STRING "" FORCE)
set(LLVM_BUILD_UTILS OFF CACHE STRING "" FORCE)
set(LLVM_BUILD_RUNTIME OFF CACHE STRING "" FORCE)
set(LLVM_BUILD_EXAMPLES OFF CACHE STRING "" FORCE)
set(LLVM_INCLUDE_TESTS OFF CACHE STRING "" FORCE)
set(LLVM_INCLUDE_UTILS OFF CACHE STRING "" FORCE)
set(LLVM_INCLUDE_RUNTIMES OFF CACHE STRING "" FORCE)
set(LLVM_INSTALL_TOOLCHAIN_ONLY ON CACHE STRING "" FORCE)
set(LLVM_INSTALL_BINUTILS_SYMLINKS ON CACHE STRING "" FORCE)
set(CLANG_INCLUDE_TESTS OFF CACHE STRING "" FORCE)

set(BUILD_SHARED_LIBS ON CACHE STRING "" FORCE)

set(LLVM_TOOLCHAIN_TOOLS
    llvm-ar
    llvm-cov
    llvm-cxxfilt
    llvm-ranlib
    llvm-lib
    llvm-nm
    llvm-objcopy
    llvm-objdump
    llvm-rc
    llvm-readobj
    llvm-size
    llvm-strings
    llvm-strip
    llvm-profdata
    llvm-symbolizer

    addr2line
    ar
    c++filt
    ranlib
    nm
    objcopy
    objdump
    size
    strings
    strip

    llvm-readelf
    readelf
)

set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
set(CMAKE_INSTALL_PREFIX ${toolchain})

if(DEFINED ENV{CCACHE_DIR})
    set(LLVM_CCACHE_BUILD ON CACHE STRING "" FORCE)
endif()

add_subdirectory(src/llvm/llvm)

include(helper_targets)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/syscalls.h
    ${CMAKE_SOURCE_DIR}/src/include/errno.h
    DESTINATION include/kernel/)
install(FILES src/include/message_passing.h DESTINATION include/sys/)
install(FILES src/include/uuid.h            DESTINATION include/)

install(FILES src/include/arch/${architecture}/io.h DESTINATION include/sys/)

configure_file(cmake/cmake.toolchain.in cmake.toolchain @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake.toolchain DESTINATION etc/)
install(FILES cmake/cmake.platform DESTINATION share/cmake/Modules/Platform RENAME "LF-OS.cmake")
