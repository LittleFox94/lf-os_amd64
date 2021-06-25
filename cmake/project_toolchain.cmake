set(lf_os_toolchain "/opt/lf_os/toolchain")
set(lf_os_sysroot   "/opt/lf_os/sysroot")

find_program(clang   NAMES clang   HINTS /usr/bin /bin /opt/lf_os/toolchain/bin)
if(clang)
    set(CMAKE_C_COMPILER   ${clang})
endif()

find_program(clangxx NAMES clang++ HINTS /usr/bin /bin /opt/lf_os/toolchain/bin)
if(clangxx)
    set(CMAKE_CXX_COMPILER ${clangxx})
endif()

find_program(lld     NAMES ld.lld  HINTS /usr/bin /bin /opt/lf_os/toolchain/bin)
if(lld)
    set(LLVM_ENABLE_LLD ON)
endif()

set(CMAKE_BUILD_TYPE Release                   CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS ON                       CACHE BOOL   "" FORCE)
set(LLVM_ENABLE_PROJECTS clang lld             CACHE STRING "" FORCE)
set(LLVM_DEFAULT_TARGET_TRIPLE x86_64-pc-lf_os CACHE STRING "" FORCE)
set(LLVM_TARGETS_TO_BUILD X86                  CACHE STRING "" FORCE)

set(LLVM_BUILD_TOOLS      ON  CACHE BOOL "" FORCE)
set(LLVM_BUILD_UTILS      OFF CACHE BOOL "" FORCE)
set(LLVM_BUILD_RUNTIME    OFF CACHE BOOL "" FORCE)
set(LLVM_BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(LLVM_INCLUDE_TESTS    OFF CACHE BOOL "" FORCE)
set(LLVM_INCLUDE_UTILS    OFF CACHE BOOL "" FORCE)
set(LLVM_INCLUDE_RUNTIMES OFF CACHE BOOL "" FORCE)
set(CLANG_INCLUDE_TESTS   OFF CACHE BOOL "" FORCE)

set(LLVM_INSTALL_TOOLCHAIN_ONLY    ON CACHE BOOL "" FORCE)
set(LLVM_INSTALL_BINUTILS_SYMLINKS ON CACHE BOOL "" FORCE)

set(CPACK_GENERATOR        "DEB;TXZ")
set(CPACK_SOURCE_GENERATOR "")
set(CPACK_PACKAGE_NAME             "lf_os-toolchain"                           CACHE STRING "" FORCE)
set(CPACK_PACKAGE_FILE_NAME        "lf_os-toolchain"                           CACHE STRING "" FORCE)
set(CPACK_PACKAGE_VENDOR           "LF OS"                                     CACHE STRING "" FORCE)
set(CPACK_PACKAGE_CONTACT          "Mara Sophie Grosch <littlefox@lf-net.org>" CACHE STRING "" FORCE)
set(CPACK_PACKAGING_INSTALL_PREFIX ${lf_os_toolchain}                          CACHE STRING "" FORCE)

set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT"              CACHE STRING "" FORCE)
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64           CACHE STRING "" FORCE)
set(CPACK_DEBIAN_COMPRESSION_TYPE xz                  CACHE STRING "" FORCE)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libtinfo6, libxml2" CACHE STRING "" FORCE)

if(DEFINED ENV{CCACHE_DIR})
    set(LLVM_CCACHE_BUILD ON CACHE STRING "" FORCE)
endif()

add_subdirectory(src/llvm/llvm)

configure_file(cmake/cmake.toolchain.in cmake.toolchain @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/cmake.toolchain DESTINATION etc/)
install(FILES cmake/cmake.platform                DESTINATION share/cmake/Modules/Platform/ RENAME LF-OS.cmake)

get_directory_property(toolchain_install DIRECTORY src/llvm/llvm DEFINITION LLVM_TOOLCHAIN_TOOLS)
list(APPEND toolchain_install clang lld)
set(toolchain_install_added ON)

while(toolchain_install_added)
    set(toolchain_install_added OFF)

    foreach(target IN ITEMS ${toolchain_install})
        if(NOT TARGET ${target})
            continue()
        endif()

        get_property(libraries TARGET ${target} PROPERTY LINK_LIBRARIES)

        foreach(lib_with_dir IN ITEMS ${libraries})
            string(REGEX REPLACE "::@.*" "" lib ${lib_with_dir})

            if(NOT TARGET ${lib})
                continue()
            endif()

            get_property(type TARGET ${lib} PROPERTY TYPE)
            get_property(name TARGET ${lib} PROPERTY NAME)

            if(${type} STREQUAL "SHARED_LIBRARY" AND NOT name IN_LIST toolchain_install)
                list(APPEND toolchain_install ${name})
                set(toolchain_install_added ON)

                install(TARGETS ${name} DESTINATION lib/)
            endif()
        endforeach()
    endforeach()
endwhile()
