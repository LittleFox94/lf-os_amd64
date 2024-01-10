project(uart CXX)

add_executable(uart main.cxx)

install(TARGETS uart DESTINATION userspace/drivers)
