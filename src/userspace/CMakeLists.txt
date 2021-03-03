foreach(program IN LISTS build_userspace)
    add_subdirectory(${program})
endforeach()
