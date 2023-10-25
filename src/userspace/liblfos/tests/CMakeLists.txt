find_package(GTest REQUIRED)
enable_language(CXX)

add_executable(liblfos-tests EXCLUDE_FROM_ALL
    dlinkedlist.cpp
    mq-router.cpp
)

target_include_directories(liblfos-tests PRIVATE ${PROJECT_SOURCE_DIR}/../../include)
target_link_libraries(liblfos-tests GTest::gtest GTest::gtest_main GTest::gmock lfos)
add_test(
    NAME liblfos
    COMMAND
        liblfos-tests
        "--gtest_output=xml:${CMAKE_BINARY_DIR}/liblfos_tests.xml"
)

add_custom_target(test-liblfos COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS liblfos-tests)
