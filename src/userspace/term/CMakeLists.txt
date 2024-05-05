cmake_minimum_required(VERSION 3.20)
project(term)

set(terminal_font "${CMAKE_CURRENT_SOURCE_DIR}/3rd-party/spleen/spleen-8x16.bdf")

include(ExternalProject)

if(CMAKE_CROSSCOMPILING)
    ExternalProject_Add(
        "native-utils"
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}"
    )
else()
    add_subdirectory(util)
endif()

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/font.c"
    COMMAND
        ${CMAKE_CURRENT_BINARY_DIR}/util/font-compile
        < "${terminal_font}"
        > "${CMAKE_CURRENT_BINARY_DIR}/font.c"
    DEPENDS
        "${terminal_font}"
        native-utils
    VERBATIM
)

add_subdirectory(3rd-party)

if(CMAKE_CROSSCOMPILING)
    add_subdirectory(../klsh klsh)

    add_custom_command(
        OUTPUT              ${PROJECT_BINARY_DIR}/bootlogo.o
        DEPENDS             ${PROJECT_SOURCE_DIR}/bootlogo.ans
        WORKING_DIRECTORY   ${PROJECT_SOURCE_DIR}
        COMMAND             ${CMAKE_OBJCOPY}
            -I binary
            -O elf64-x86-64
            bootlogo.ans
            ${PROJECT_BINARY_DIR}/bootlogo.o
    )

    add_executable(term
        main.c      main.h
        keyboard.c  keyboard.h
        rw_hook.c

        "${PROJECT_BINARY_DIR}/bootlogo.o"
        "${CMAKE_CURRENT_BINARY_DIR}/font.c"
    )
    target_link_libraries(term PUBLIC vterm klsh)
    target_include_directories(term PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_options(term PRIVATE -Wl,--allow-multiple-definition)
    install(TARGETS term DESTINATION userspace)
endif()

