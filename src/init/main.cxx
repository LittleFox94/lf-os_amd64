#include <iostream>
#include <unistd.h>
#include <cstdint>

typedef uint64_t ptr_t;

#include <kernel/syscalls.h>

int main(int argc, char* argv[]) {
    uint64_t framebuffer;
    uint16_t width, height, stride, colorFormat;
    sc_do_hardware_framebuffer(&framebuffer, &width, &height, &stride, &colorFormat);

    volatile uint32_t* fb = (uint32_t*)framebuffer;

    for(int i = 0; i < 4; ++i) {
        int pid = fork();

        if(pid == 0) {
            uint16_t xstart = (i % 2) * (width / 2);
            uint16_t ystart = (i / 2) * (height / 2);

            uint32_t color;

            while(true) {
                for(size_t y = 0; y < height / 2; ++y) {
                    for(size_t x = 0; x < width / 2; ++x) {
                        fb[(ystart + y) * width + xstart + x] = color;
                    }
                }

                color += 80;
            }
        }
    }


    return EXIT_SUCCESS;
}
