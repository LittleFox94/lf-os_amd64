add_executable(hello EXCLUDE_FROM_ALL hello.c)
enable_language(C)

target_include_directories(hello PRIVATE ../../lib/include)
target_link_libraries(hello      PRIVATE 9p)
