cmake_minimum_required(VERSION 3.10)
project(loader LANGUAGES C VERSION 0.0.1)

if(${architecture} STREQUAL "amd64")
    set(loader_architecture "x86_64")
else()
    set(loader_architecture ${architecture})
endif()

add_executable(loader
    loader.c
    string.c
)

configure_file(config.h.in config.h)
target_include_directories(loader PRIVATE
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/../include"
    "${PROJECT_SOURCE_DIR}/../include/efi"
)

target_compile_options(loader PRIVATE
    -Wall -Werror
    -std=gnu99
    -target ${loader_architecture}-unknown-windows
    -ffreestanding -fno-stack-protector
    -mno-red-zone
    -gdwarf
    -mno-ms-bitfields
    -O0 # ¯\_(ツ)_/¯
)

target_link_options(loader PRIVATE
    -std=gnu99
    -target ${loader_architecture}-unknown-windows
    -Wl,/debug:dwarf
    -Wl,-nodefaultlib
    -Wl,-dll
    -Wl,-entry:efi_main
    -Wl,-subsystem:efi_application
    -fuse-ld=lld-link
    -mno-ms-bitfields
    -O0 # ¯\_(ツ)_/¯
)

set_target_properties(loader PROPERTIES OUTPUT_NAME loader.efi)

install(FILES $<TARGET_FILE:loader> DESTINATION EFI/LFOS RENAME BOOTX64.efi)
install(FILES $<TARGET_FILE:loader> DESTINATION EFI/BOOT RENAME BOOTX64.efi)
