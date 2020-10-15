cmake_minimum_required(VERSION 3.16)
project(kernel_tests)

include(CTest)

add_executable(test_runner runner.cxx)
target_link_libraries(test_runner dl)
add_custom_target(test-kernel COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS test_runner)

file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test_*.c)
foreach(test IN LISTS test_files)
    string(REGEX REPLACE "^test_(.*).c$" "\\1" test ${test})
    add_library(kerneltest-${test} SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.c
    )
    target_include_directories(kerneltest-${test}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../
            ${CMAKE_CURRENT_SOURCE_DIR}/../arch/amd64
    )
    target_compile_options(kerneltest-${test}
        PRIVATE -ffreestanding -nostdlib -nodefaultlibs -static -fvisibility=hidden -nostdinc
    )

    add_test(
        NAME ${test}
        COMMAND
            test_runner
            $<TARGET_FILE:kerneltest-${test}>
    )
    add_dependencies(test-kernel kerneltest-${test})
endforeach()
