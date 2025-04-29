cmake_minimum_required(VERSION 3.20)
project(tinystl LANGUAGES CXX)

add_library(tinystl-helpers STATIC
    include/helpers/tinystl/binary_tree
)
target_include_directories(tinystl-helpers PUBLIC include/helpers)

add_library(tinystl STATIC
    include/std/bitset
    include/std/cstdint
    include/std/cstddef
    include/std/cstring
    include/std/forward_list
    include/std/functional
    include/std/initializer_list
    include/std/map
    include/std/memory
    include/std/new
    include/std/utility
)

set_target_properties(tinystl tinystl-helpers PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(tinystl      PUBLIC tinystl-helpers)
target_include_directories(tinystl PUBLIC include/std)

include(CTest)

if(${BUILD_TESTING})
    find_package(GTest REQUIRED)
    add_executable(tinystl_tests EXCLUDE_FROM_ALL
        tests/bitset.cpp
        tests/forward_list.cpp
        tests/map.cpp
    )

    target_compile_options(tinystl_tests PRIVATE -Wall -Werror -Wpedantic)
    target_compile_features(tinystl_tests PRIVATE cxx_std_20)
    target_link_libraries(tinystl_tests tinystl-helpers GTest::GTest GTest::gtest_main)

    add_test(
        NAME tinystl_tests
        COMMAND
            $<TARGET_FILE:tinystl_tests>
            "--gtest_output=xml:${CMAKE_BINARY_DIR}/tinystl_tests.xml"
    )

    add_custom_target(test-tinystl COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS tinystl_tests)
endif()
