function(multistage)
    # Staging logic module for cmake
    set(current_stage "root" CACHE STRING "Current build stage (you want to start with root)")

    if(staging_shared_install)
        set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/shared)
    endif()

    if(${current_stage} STREQUAL "root")
        include(ExternalProject)

        foreach(s IN LISTS ARGV)
            set(inner_cmake_args
                "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}"
                "-Dcurrent_stage:STRING=${s}"
            )

            set(stage_safe_vars
                CMAKE_INSTALL_PREFIX      CMAKE_TOOLCHAIN_FILE
                CMAKE_C_COMPILER_LAUNCHER CMAKE_CXX_COMPILER_LAUNCHER
            )

            foreach(var IN LISTS stage_safe_vars)
                if(DEFINED ${var})
                    set(inner_cmake_args ${inner_cmake_args} "-D${var}:STRING=${${var}}")
                endif()
            endforeach()

            if(COMMAND stage_configure)
                stage_configure(stage_cmake_args)
                set(inner_cmake_args ${stage_cmake_args} ${inner_cmake_args})
            endif()

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
    else()
        if(CMAKE_BUILD_TYPE)
            string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UC)
        endif()

        set(FULL_C_FLAGS        "${CMAKE_C_FLAGS}       ${CMAKE_C_FLAGS_${BUILD_TYPE_UC}}")
        set(FULL_CXX_FLAGS      "${CMAKE_CXX_FLAGS}     ${CMAKE_CXX_FLAGS_${BUILD_TYPE_UC}}")
        set(FULL_LINKER_FLAGS   "${CMAKE_LINKER_FLAGS}  ${CMAKE_LINKER_FLAGS_${BUILD_TYPE_UC}}")

        include("stage_${current_stage}")
    endif()
endfunction()
