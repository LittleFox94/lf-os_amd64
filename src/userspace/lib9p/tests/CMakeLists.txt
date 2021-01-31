find_package(GTest REQUIRED)

add_executable(lib9p-tests
    decode.cpp
    wireutils.cpp
)

target_link_libraries(lib9p-tests GTest::GTest GTest::Main 9p)
target_include_directories(lib9p-tests PRIVATE "${PROJECT_SOURCE_DIR}/lib/include")
target_compile_options(lib9p-tests PRIVATE -Wno-narrowing)

add_test(
    NAME lib9p
    COMMAND
        lib9p-tests
        "--gtest_output=xml:${CMAKE_BINARY_DIR}/lib9p_tests.xml"
)

add_custom_target(test-lib9p COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS lib9p-tests)
