enable_language(CXX)

set(INSTALL_GTEST OFF)
option(gtest_disable_pthreads "Disable uses of pthreads in gtest." ON)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/googletest gtest)

target_compile_definitions(
    gtest
    PUBLIC
        "-DGTEST_HAS_STREAM_REDIRECTION=0"
        "-DGTEST_HAS_POSIX_RE=0"
        "-DGTEST_HAS_FILE_SYSTEM=0"
)

add_custom_target(test-runtime COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS kernel)

file(GLOB_RECURSE test_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS *.cxx)

foreach(test IN LISTS test_files)
    if(${test} STREQUAL "gtest_main.cxx")
        continue()
    endif()

    string(REGEX REPLACE "^(.*).cxx$" "\\1" test ${test})

    add_executable(
        ${test}
        ${CMAKE_CURRENT_SOURCE_DIR}/${test}.cxx

        ${CMAKE_CURRENT_SOURCE_DIR}/gtest_main.cxx
    )

    target_link_libraries(
        ${test}
        gtest
    )

    install(TARGETS ${test} DESTINATION runtime-tests)
endforeach()

