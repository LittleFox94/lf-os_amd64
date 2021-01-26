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
    efi.c       efi.h
    elf.c       elf.h
    fbconsole.c fbconsole.h
    log.c       log.h
    mutex.c     mutex.h
    sd.c        sd.h
    slab.c      slab.h
    string.c    string.h
    tpa.c       tpa.h
    flexarray.c flexarray.h
    mq.c        mq.h
    version.c
                allocator.h
                bitmap.h
                stdarg.h
                stdbool.h
    ${PROJECT_BINARY_DIR}/syscalls.gen.c
)

add_subdirectory("arch/${architecture}")

configure_file(config.h.in config.h)
target_include_directories(kernel PUBLIC
    "${PROJECT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/arch/${architecture}"
    "${PROJECT_SOURCE_DIR}/../include"
    "${PROJECT_SOURCE_DIR}/../include/efi"
    "${PROJECT_SOURCE_DIR}/../include/arch/${architecture}"
)

target_compile_options(kernel PRIVATE
    -Wall -Werror -std=gnu11
    -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer -fno-stack-protector -mno-red-zone
    -fno-builtin -ffreestanding -nostdinc -fshort-wchar
    -mcmodel=large
    -flto
)

target_link_options(kernel PRIVATE
    -flto -fuse-ld=lld -nostdlib -Wl,--image-base,0xFFFF800001000000
)

install(TARGETS kernel DESTINATION LFOS)
