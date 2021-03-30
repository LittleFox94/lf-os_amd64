# LF OS build settings
lfos_config(architecture "amd64"                         "Architecture to build for (only amd64 for now)")
lfos_config(toolchain    "${CMAKE_SOURCE_DIR}/toolchain" "LF OS toolchain")

lfos_config(kernel_log_max_buffer  "4*1024*1024" "Max size of buffer storing log messages")
lfos_config(kernel_log_com0        "true"        "Log messages to platform first serial port")
lfos_config(kernel_log_efi         "true"        "Log messages to EFI firmware variables to be persisted through reboots")

lfos_config(loader_lfos_path       "LFOS"        "Where shall the loader search for kernel and other files")
lfos_config(loader_efi_rt_services 0             "Enable EFI runtime services")

lfos_config(build_userspace "init;drivers/uart" "Userspace programs to build and install")
