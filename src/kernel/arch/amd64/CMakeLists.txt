target_sources(kernel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/panic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdt.S
    ${CMAKE_CURRENT_SOURCE_DIR}/init.S
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/msr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scheduler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vm.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/memory/context.cpp
)
