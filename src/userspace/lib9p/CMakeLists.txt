cmake_minimum_required(VERSION 3.10)
project(lib9p CXX)

include(CTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

add_subdirectory(lib)
add_subdirectory(example/fuse)

if(${BUILD_TESTING})
    enable_testing()
    add_subdirectory(tests)
endif()
