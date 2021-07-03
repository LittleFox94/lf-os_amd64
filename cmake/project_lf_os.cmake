set(lf_os_sysroot_external OFF)
if(DEFINED lf_os_sysroot AND NOT DEFINED current_stage)
    set(lf_os_sysroot_external ON)
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
    include(tests)
    include(images)
    include(helper_targets)
    add_dependencies(hd.img stage_userspace)
endif()
