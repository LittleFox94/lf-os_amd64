cmake_minimum_required(VERSION 3.10)
project(kernel LANGUAGES CXX ASM VERSION 0.0.1)

add_custom_command(
    OUTPUT            syscalls.gen.cpp
    COMMAND           ../syscall-generator.pl ARGS ../syscalls.yml ${PROJECT_BINARY_DIR}/syscalls.gen.cpp kernel
    MAIN_DEPENDENCY   ../syscalls.yml
    DEPENDS           ../syscall-generator.pl
    COMMENT           "Generating syscall handling code"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_executable(kernel
    condvar.cpp   condvar.h
    cpp_runtime.cpp
    elf.cpp       elf.h
    flexarray.cpp flexarray.h
    log.cpp       log.h
    mm.cpp        mm.h
    mq.cpp        mq.h
    mutex.cpp     mutex.h
    sd.cpp        sd.h
    slab.cpp      slab.h
    string.cpp    cstdlib/string.h
                  tpa.h
    uuid.cpp      ../include/uuid.h
    version.cpp
                        allocator.h
                        allocator/base.h
    allocator/page.cpp  allocator/page.h
                        allocator/sized.h
                        allocator/typed.h
                        bitmap.h
                        bitset_helpers.h
    ${PROJECT_BINARY_DIR}/syscalls.gen.cpp
    ${PROJECT_BINARY_DIR}/config.h

    drivers/acpi.cpp      drivers/acpi.h
    drivers/efi.cpp       drivers/efi.h
    drivers/fbconsole.cpp drivers/fbconsole.h
    drivers/hpet.cpp      drivers/hpet.h
)

add_subdirectory(../lib/tiny-stl tinystl)
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

target_link_libraries(kernel PRIVATE tinystl)

target_compile_definitions(kernel PRIVATE __kernel)

target_compile_options(kernel PRIVATE
    -target x86_64-unknown-elf
    -Wall -Wextra -Wpedantic -Werror -std=c++23
    -Wno-gnu-zero-variadic-macro-arguments
    -Wno-gnu-include-next
    -Wno-zero-length-array
    -Wno-language-extension-token
    -Wno-gnu-folding-constant
    -Wno-overlength-strings
    -mno-omit-leaf-frame-pointer -fno-omit-frame-pointer
    -fno-stack-protector -mno-red-zone
    -fno-exceptions
    -fno-rtti
    -fno-builtin
    -ffreestanding
    -fshort-wchar
    -mcmodel=kernel
    -flto
)

target_link_options(kernel PRIVATE
    -flto -fuse-ld=lld -nostdlib -Wl,--image-base,0xffffffff81000000
)

install(TARGETS kernel DESTINATION kernel)
