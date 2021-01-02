project(init CXX)

find_package(Qt5Widgets)

add_executable(init main.cxx)
target_include_directories(init PRIVATE ${Qt5Widgets_HEADERS})
target_link_libraries(init ${Qt5Widgets_LIBRARIES} pthread)

install(TARGETS init DESTINATION LFOS)
