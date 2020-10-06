add_subdirectory(src/loader)
add_subdirectory(src/kernel)

include(ExternalProject)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -nostdlib")

ExternalProject_Add(
    "compiler-rt"
    CMAKE_CACHE_ARGS
        "-DCOMPILER_RT_DEFAULT_TARGET_ONLY:BOOL=ON"
        "-DCOMPILER_RT_OUTPUT_DIR:STRING=${toolchain}/lib/clang/11.0.0"
        "-DCOMPILER_RT_ENABLE_SHARED:STRING=OFF"
        "-DCOMPILER_RT_USE_LIBCXX:STRING=OFF"
        "-DCOMPILER_RT_CRT_USE_EH_FRAME_REGISTRY:STRING=OFF"
        "-DCOMPILER_RT_EXLUCE_ATOMIC_BUILTIN:STRING=OFF"
        "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS}"
        "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}"
        "-DCMAKE_TOOLCHAIN_FILE:STRING=${toolchain}/etc/cmake.toolchain"
        "-DCMAKE_INSTALL_PREFIX:STRING=${toolchain}"
    SOURCE_DIR
        "${CMAKE_SOURCE_DIR}/src/llvm/compiler-rt"
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD     ON
    USES_TERMINAL_INSTALL   ON
)
