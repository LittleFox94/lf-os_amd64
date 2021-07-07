cmake_minimum_required(VERSION 3.10)
project(kernel_tests)

find_package(GTest REQUIRED)

include(CTest)

add_executable(test_runner runner.cxx)
target_link_libraries(test_runner GTest::GTest dl)
target_link_options(test_runner PRIVATE -Wl,-E)
target_compile_options(test_runner PRIVATE -fvisibility=hidden)

add_custom_target(test-kernel COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS test_runner)

file(GLOB test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS test_*.cxx)

set(tests "")

foreach(test IN LISTS test_files)
    string(REGEX REPLACE "^test_(.*).cxx$" "\\1" test ${test})

    add_library(
        kerneltest-${test}
        SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/test_${test}.cxx
    )

    target_include_directories(
        kerneltest-${test}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/../../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../
            ${CMAKE_CURRENT_SOURCE_DIR}/../arch/amd64
            ${GTEST_INCLUDE_DIR}
    )

    target_compile_options(
        kerneltest-${test}
        PRIVATE
            -nostdlib -nodefaultlibs -static -fvisibility=hidden -fno-builtin -fpermissive -Wno-write-strings
    )

    list(APPEND tests $<TARGET_FILE:kerneltest-${test}>)

    add_dependencies(test-kernel kerneltest-${test})
endforeach()

add_test(
    NAME kernel_tests
    COMMAND
        test_runner
        ${tests}
        "--gtest_output=xml:${CMAKE_BINARY_DIR}/kernel_tests.xml"
)
