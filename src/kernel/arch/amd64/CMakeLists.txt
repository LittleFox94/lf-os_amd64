target_sources(kernel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/apic/apic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/apic/io.c
    ${CMAKE_CURRENT_SOURCE_DIR}/apic/timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/apic/legacy.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bluescreen.c
    ${CMAKE_CURRENT_SOURCE_DIR}/gdt.S
    ${CMAKE_CURRENT_SOURCE_DIR}/init.S
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/msr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/sc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/scheduler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/vm.c
)
