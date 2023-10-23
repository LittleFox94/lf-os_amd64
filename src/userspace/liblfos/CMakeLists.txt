project(liblfos LANGUAGES C)

add_library(
    lfos

    lib/mq-router.c
    lib/dlinkedlist.c
)
set(
    lfos_headers

    include/lfos/mq-router.h
    include/lfos/dlinkedlist.h
)

target_sources(lfos
    PUBLIC
    FILE_SET  headers
    TYPE      HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include
    FILES     ${lfos_headers}
)
target_include_directories(lfos PRIVATE ${PROJECT_SOURCE_DIR}/include)

install(TARGETS lfos                  DESTINATION "${lf_os_sysroot}/lib")
install(TARGETS lfos FILE_SET headers DESTINATION "${lf_os_sysroot}/include")
