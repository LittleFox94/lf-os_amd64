project(init CXX)
add_executable(init main.cxx)
target_link_libraries(init pthread)
install(TARGETS init DESTINATION LFOS)
