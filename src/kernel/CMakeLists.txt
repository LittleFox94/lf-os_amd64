cmake_minimum_required(VERSION 3.10)
project(kernel LANGUAGES C ASM VERSION 0.0.1)

add_custom_command(
    OUTPUT            syscalls.gen.c
    COMMAND           ../syscall-generator.pl ARGS ../syscalls.yml ${PROJECT_BINARY_DIR}/syscalls.gen.c kernel
    MAIN_DEPENDENCY   ../syscalls.yml
    DEPENDS           ../syscall-generator.pl
    COMMENT           "Generating syscall handling code"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_executable(kernel
    condvar.c   condvar.h
    elf.c       elf.h
    flexarray.c flexarray.h
    log.c       log.h
    mm.c        mm.h
    mq.c        mq.h
    mutex.c     mutex.h
    sd.c        sd.h
    slab.c      slab.h
    string.c    cstdlib/string.h
    tpa.c       tpa.h
    uuid.c      ../include/uuid.h
    version.c
                allocator.h
                bitmap.h
                cstdlib/stdarg.h
                cstdlib/stdbool.h
    ${PROJECT_BINARY_DIR}/syscalls.gen.c
    ${PROJECT_BINARY_DIR}/config.h

    drivers/acpi.c      drivers/acpi.h
    drivers/efi.c       drivers/efi.h
    drivers/fbconsole.c drivers/fbconsole.h
    drivers/hpet.c      drivers/hpet.h
)

add_subdirectory("arch/${architecture}")

configure_file(config.h.in config.h)
target_include_directories(kernel PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/drivers"
    "${PROJECT_SOURCE_DIR}/cstdlib"
    "${PROJECT_SOURCE_DIR}/arch/${architecture}"
    "${PROJECT_SOURCE_DIR}/../include"
    "${PROJECT_SOURCE_DIR}/../include/sys"
    "${PROJECT_SOURCE_DIR}/../include/efi"
    "${PROJECT_SOURCE_DIR}/../include/arch/${architecture}"
)

target_compile_definitions(kernel PRIVATE __kernel)

target_compile_options(kernel PRIVATE
    -Wall -Wextra -Wpedantic -Werror -std=gnu11
    -Wno-gnu-zero-variadic-macro-arguments
    -Wno-zero-length-array
    -Wno-language-extension-token
    -Wno-gnu-folding-constant
    -Wno-overlength-strings
    -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer
    -fno-stack-protector -mno-red-zone
    -fno-builtin -ffreestanding -nostdinc -fshort-wchar
    -mcmodel=kernel
    -flto
)

target_link_options(kernel PRIVATE
    -flto -fuse-ld=lld -nostdlib -Wl,--image-base,0xffffffff81000000
)

install(TARGETS kernel DESTINATION kernel)
