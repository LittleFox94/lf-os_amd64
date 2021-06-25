set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -nostdlib")

add_subdirectory(src/loader)
add_subdirectory(src/kernel)
