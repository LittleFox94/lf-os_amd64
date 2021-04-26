cmake_minimum_required(VERSION 3.10)
include(ExternalProject)

ExternalProject_Add(
    "compiler-rt"
    CMAKE_CACHE_ARGS
        "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_TOOLCHAIN_FILE:STRING=${toolchain}/etc/cmake.toolchain"
        "-DCMAKE_INSTALL_PREFIX:STRING=${toolchain}/lib/clang/13.0.0"
        "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS} -nostdlib"
        "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS} -nostdlib"
        "-DCOMPILER_RT_DEFAULT_TARGET_ONLY:BOOL=ON"
        "-DCOMPILER_RT_ENABLE_SHARED:STRING=OFF"
        "-DCOMPILER_RT_USE_LIBCXX:STRING=ON"
        "-DCOMPILER_RT_CRT_USE_EH_FRAME_REGISTRY:STRING=OFF"
        "-DCOMPILER_RT_EXCLUDE_ATOMIC_BUILTIN:STRING=OFF"
    SOURCE_DIR
        "${CMAKE_SOURCE_DIR}/src/llvm/compiler-rt"
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD     ON
    USES_TERMINAL_INSTALL   ON
    BUILD_ALWAYS            ON
)

ExternalProject_Add(
    "libc++"
    CMAKE_CACHE_ARGS
        "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_TOOLCHAIN_FILE:STRING=${toolchain}/etc/cmake.toolchain"
        "-DCMAKE_INSTALL_PREFIX:STRING=${toolchain}"
        "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS} -nostdlib"
        "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS} -nostdlib"
        "-DLIBCXX_ENABLE_THREADS:STRING=On"
        "-DLIBCXX_ENABLE_MONOTONIC_CLOCK:STRING=On"
        "-DLIBCXX_ENABLE_FILESYSTEM:STRING=Off"
        "-DLIBCXX_ENABLE_STATIC_ABI_LIBRARY:STRING=On"
        "-DLIBCXX_ENABLE_SHARED:STRING=Off"
        "-DLIBCXX_USE_COMPILER_RT:STRING=On"
        "-DLIBCXX_CXX_ABI:STRING=libcxxabi"
        "-DLIBCXX_CXX_STATIC_ABI_LIBRARY:STRING=c++abi"
        "-DLIBCXX_CXX_ABI_LIBRARY_PATH:STRING=${toolchain}/lib"
        "-DLIBCXX_INCLUDE_TESTS:STRING=Off"
    SOURCE_DIR
        "${CMAKE_SOURCE_DIR}/src/llvm/libcxx"
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD     ON
    USES_TERMINAL_INSTALL   ON
    BUILD_ALWAYS            ON
    STEP_TARGETS
        "build"
        "install-headers"
)
ExternalProject_Add_Step("libc++" "install-headers"
    COMMAND ${CMAKE_MAKE_PROGRAM} install-cxx-headers
    WORKING_DIRECTORY <BINARY_DIR>
)

ExternalProject_Add(
    "libc++abi"
    DEPENDS
        "compiler-rt"
        "libc++-install-headers"
    CMAKE_CACHE_ARGS
        "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
        "-DCMAKE_TOOLCHAIN_FILE:STRING=${toolchain}/etc/cmake.toolchain"
        "-DCMAKE_INSTALL_PREFIX:STRING=${toolchain}"
        "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}"
        "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}"
        "-DLIBCXXABI_ENABLE_THREADS:STRING=Off"
        "-DLIBCXXABI_USE_COMPILER_RT:STRING=On"
        "-DLIBCXXABI_ENABLE_SHARED:STRING=Off"
        "-DLIBCXXABI_ENABLE_STATIC_UNWINDER:STRING=On"
        "-DLIBCXXABI_USE_LLVM_UNWINDER:STRING=On"
        "-DLIBCXXABI_LIBCXX_INCLUDES:STRING=${toolchain}/include/c++/v1"
        "-DLIBUNWIND_ENABLE_SHARED:STRING=Off"
        "-DLIBUNWIND_COMPILE_FLAGS:STRING="
    SOURCE_DIR
        "${CMAKE_SOURCE_DIR}/src/llvm/libcxxabi"
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD     ON
    USES_TERMINAL_INSTALL   ON
    BUILD_ALWAYS            ON
    STEP_TARGETS
        "install"
)

ExternalProject_Add_StepDependencies("libc++" "build" "libc++abi")

install(CODE "") # nothing to install
