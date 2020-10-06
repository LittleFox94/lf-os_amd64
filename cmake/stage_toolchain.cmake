cmake_minimum_required(VERSION 3.10)
project(toolchain C CXX ASM)

set(LLVM_ENABLE_PROJECTS "clang;lld")
set(LLVM_DEFAULT_TARGET_TRIPLE "x86_64-pc-lf_os")
set(LLVM_TARGETS_TO_BUILD "X86")
set(LLVM_BUILD_TOOLS ON)
set(LLVM_BUILD_UTILS ON)
set(LLVM_BUILD_RUNTIMES ON)
set(LLVM_BUILD_RUNTIME ON)
set(LLVM_USE_LIBCXX ON)

set(CMAKE_C_COMPILER_NAMES   clang   gcc)
set(CMAKE_CXX_COMPILER_NAMES clang++ g++)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_INSTALL_PREFIX ${toolchain})

add_subdirectory(src/llvm/llvm)

add_custom_target(
    syscalls.h  ALL
    ${PROJECT_SOURCE_DIR}/src/syscall-generator.pl ${PROJECT_SOURCE_DIR}/src/syscalls.yml ${PROJECT_BINARY_DIR}/syscalls.h user
    SOURCES
        ${PROJECT_SOURCE_DIR}/src/syscalls.yml
        ${PROJECT_SOURCE_DIR}/src/syscall-generator.pl
    COMMENT           "Generating userspace syscall bindings"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/syscalls.h DESTINATION include/kernel/)

configure_file(cmake/cmake.toolchain.in cmake.toolchain @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cmake.toolchain DESTINATION etc/)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake.platform DESTINATION share/cmake/Modules/Platform RENAME "LF-OS.cmake")
