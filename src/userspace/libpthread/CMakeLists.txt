cmake_minimum_required(VERSION 3.10)

project(libpthread LANGUAGES C VERSION 0.0.1)
add_library(pthread
    pthread.c
    pthread_cleanup.c
    pthread_cond.c
    pthread_mutex.c
    pthread_key.c
)
install(TARGETS pthread DESTINATION "${lf_os_sysroot}/lib")
