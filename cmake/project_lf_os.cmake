include(config)

set(staging_shared_install ON)
include(staging)
multistage(sysroot libc stdlibs lowlevel userspace)

if(${current_stage} STREQUAL "root")
    include(tests)
    include(images)
    include(helper_targets)
    add_dependencies(hd.img stage_userspace)
endif()
