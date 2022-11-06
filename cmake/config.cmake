function(lfos_config name default desc)
    set("${name}" "${default}" CACHE STRING "${desc}")
    set(lfos_vars ${lfos_vars} ${name} PARENT_SCOPE)
endfunction()

# LF OS build settings
lfos_config(architecture    "amd64"                       "Architecture to build for (only amd64 for now)")
lfos_config(lf_os_toolchain "/opt/lf_os/toolchain"        "LF OS toolchain location")
lfos_config(lf_os_sysroot   "${CMAKE_BINARY_DIR}/sysroot" "LF OS sysroot location")

lfos_config(kernel_log_max_buffer  "4*1024*1024" "Max size of buffer storing log messages")
lfos_config(kernel_log_com0        "true"        "Log messages to platform first serial port")
lfos_config(kernel_log_efi         "false"       "Log messages to EFI firmware variables to be persisted through reboots")

lfos_config(loader_lfos_path       "LFOS"        "Where shall the loader search for kernel and other files")
lfos_config(loader_efi_rt_services 1             "Enable EFI runtime services")

lfos_config(build_userspace "fbdemo;drivers/uart" "Userspace programs to build and install")

function(stage_configure dest)
    foreach(var IN ITEMS ${lfos_vars} subproject)
        set(vars ${vars} "-D${var}:STRING=${${var}}")
    endforeach()

    set(${dest} ${vars} "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${lf_os_toolchain}/etc/cmake.toolchain" PARENT_SCOPE)
endfunction()
