add_library(9p
    qid.cpp
    stat.cpp
    wireutils.cpp
    transport.cpp
    connection.cpp

    message.cpp

    helper/filesystem.cpp
    helper/server_starter.cpp
    helper/client.cpp

    c/fuse.cpp
)
set(lib9p_headers
    include/9p/qid
    include/9p/stat
    include/9p/wireutils
    include/9p/transport
    include/9p/connection
    include/9p/exception

    include/9p/message

    include/9p/helper/filesystem
    include/9p/helper/server_starter
    include/9p/helper/client

    include/9p/fuse.h
)
target_compile_definitions(9p PRIVATE _LIB9P_INTERNALS=1)

target_include_directories(9p PUBLIC include)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "LF-OS")
    target_sources(9p PRIVATE
        transport/tcp.cpp
    )
    list(APPEND lib9p_headers include/9p/transports/tcp)
else()
    target_sources(9p PRIVATE
        transport/lf_os.cpp
        c/stdlib.cpp
    )
    list(APPEND lib9p_headers include/9p/transports/lf_os)
endif()

target_sources(9p
    PUBLIC
    FILE_SET  headers
    TYPE      HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/lib/include
    FILES     ${lib9p_headers}
)

target_link_libraries(9p pthread)

target_compile_features(9p PUBLIC cxx_std_20)
