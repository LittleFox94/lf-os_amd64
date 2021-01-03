cmake_minimum_required(VERSION 3.10)
project("lf_os" NONE)

# This is a multi-stage CMake build.
# Since when does CMake support multi-stage builds you may ask. Answer: it does not.
# This CMakeLists is called recursively as external_project with different settings
# given on each run. It only contains the common LF OS build settings and the staging-
# logic.
# The stages are in cmake/stage_$n.cmake

# LF OS build settings
set(architecture "amd64"                      CACHE STRING "Architecture to build for (only amd64 for now)")
set(toolchain "${CMAKE_SOURCE_DIR}/toolchain" CACHE PATH   "LF OS toolchain")

set(kernel_log_max_buffer "4*1024*1024" CACHE STRING "Max size of buffer storing log messages")
set(kernel_log_com0       "true"        CACHE STRING "Log messages to platform first serial port")

set(loader_lfos_path      "LFOS" CACHE STRING "Where shall the loader search for kernel and other files")

set(lfos_vars
    architeture
    toolchain

    kernel_log_max_buffer
    kernel_log_com0
    loader_lfos_path
)

# CMake settings for LF OS
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" CACHE PATH "CMake module path for LF OS" FORCE)

set(CMAKE_C_FLAGS         "-Wno-gnu-folding-constant")
set(CMAKE_C_FLAGS_DEBUG   "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS       "-Wno-gnu-folding-constant")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

# Staging logic
set(stage "root" CACHE STRING "Current build stage (you want to start with root)")

if(${stage} STREQUAL "root")
    set(all_stages toolchain libc stdlibs lowlevel userspace)
    include(ExternalProject)

    foreach(s IN LISTS all_stages)
        if(s STREQUAL "toolchain")
            set(tcf
                "-DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}"
                "-DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}"
            )
        else()
            set(tcf "-DCMAKE_TOOLCHAIN_FILE:STRING=${toolchain}/etc/cmake.toolchain")
        endif()

        set(inner_cmake_args
            "-DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_BINARY_DIR}/shared"
            "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
            "-DCMAKE_C_COMPILER_LAUNCHER:STRING=${CMAKE_C_COMPILER_LAUNCHER}"
            "-DCMAKE_CXX_COMPILER_LAUNCHER:STRING=${CMAKE_CXX_COMPILER_LAUNCHER}"
            "-Dstage:STRING=${s}"
            ${tcf}
        )

        foreach(var IN LISTS lfos_vars)
            set(inner_cmake_args ${inner_cmake_args} "-D${var}:STRING=${${var}}")
        endforeach()

        ExternalProject_Add(
            "stage_${s}"
            CMAKE_CACHE_ARGS
                ${inner_cmake_args}
            SOURCE_DIR
                "${CMAKE_SOURCE_DIR}"
            DEPENDS
                ${prev_stage}
            USES_TERMINAL_CONFIGURE ON
            USES_TERMINAL_BUILD     ON
            USES_TERMINAL_INSTALL   ON
            BUILD_ALWAYS            ON
        )

        set(prev_stage "stage_${s}")
    endforeach()

    include(tests)
    include(images)
    include(helper_targets)
    add_dependencies(hd.img stage_userspace)
else()
    include("stage_${stage}")
endif()
