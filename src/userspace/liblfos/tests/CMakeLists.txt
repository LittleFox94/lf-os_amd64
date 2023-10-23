find_package(GTest REQUIRED)
enable_language(CXX)

add_executable(liblfos-tests EXCLUDE_FROM_ALL
    dlinkedlist.cpp
)

target_link_libraries(liblfos-tests GTest::GTest GTest::Main lfos)
add_test(NAME liblfos COMMAND liblfos-tests)
