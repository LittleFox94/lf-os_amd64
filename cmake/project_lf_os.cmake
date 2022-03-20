set(lf_os_sysroot_external ON)

if(DEFINED lf_os_sysroot)
    cmake_path(GET lf_os_sysroot PARENT_PATH sysroot_parent_dir)

    if(sysroot_parent_dir STREQUAL ${CMAKE_BINARY_DIR})
        set(lf_os_sysroot_external OFF)
    endif()
else()
    set(lf_os_sysroot_external OFF)
endif()

include(config)

set(staging_shared_install ON)
include(staging)

set(stages lowlevel userspace)

if(NOT lf_os_sysroot_external)
    list(PREPEND stages sysroot libc stdlibs)
endif()

multistage(${stages})

if(${current_stage} STREQUAL "root")
    enable_language(C CXX)
    add_subdirectory(util)

    include(tests)
    include(images)
    include(helper_targets)
    add_dependencies(bootfs.img stage_userspace)
endif()
