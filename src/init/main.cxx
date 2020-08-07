#include <iostream>
#include <unistd.h>
#include <cstdint>

#define _POSIX_THREADS
#include <pthread.h>

typedef uint64_t ptr_t;

#include <kernel/syscalls.h>

int main(int argc, char* argv[]) {
    uint32_t* fb;
    uint16_t width, height, stride, colorFormat;
    sc_do_hardware_framebuffer((uint64_t*)&fb, &width, &height, &stride, &colorFormat);

    pthread_mutex_t fb_mutex;
    pthread_mutex_init(&fb_mutex, 0);
    int error;

    for(int i = 0; i < 16; ++i) {
        int pid = fork();

        if(pid == 0) {
            const uint16_t xstart = (i % 4) * (width  / 4);
            const uint16_t ystart = (i / 4) * (height / 4);

            uint32_t color;

            while(true) {
                if((error = pthread_mutex_lock(&fb_mutex)) != 0) {
                    printf("lock error: %d", errno);
                }

                for(size_t y = 0; y < height / 4; ++y) {
                    for(size_t x = 0; x < width / 4; ++x) {
                        fb[(ystart + y) * width + xstart + x] = color;
                    }
                }

                color++;

                if((error = pthread_mutex_unlock(&fb_mutex)) != 0) {
                    printf("unlock error: %d", errno);
                }
            }
        }
    }


    return EXIT_SUCCESS;
}
