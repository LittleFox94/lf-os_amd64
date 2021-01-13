#include <iostream>
#include <unistd.h>
#include <cstdint>
#include <cmath>
#include <mutex>

#if   defined(__LF_OS__)
#include <kernel/syscalls.h>
#elif defined(__linux)
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

void sc_do_hardware_framebuffer(uint32_t** fb, uint16_t* width, uint16_t* height, uint16_t* stride, uint16_t* color) {
    *width = 800;
    *height = 600;
    *stride = 800;

    int fd = open("framebuffer.bmp", O_CREAT|O_RDWR, 0644);

    // BMP file header
    write(fd, "BM", 2); // magic
    uint32_t i = (*width * *height * 4) + 4096;
    write(fd, &i, 4); // file size
    i = 0;
    write(fd, &i, 4); // reserved
    i = 4096;
    write(fd, &i, 4); // image data offset

    // BMP info header
    i = 40;
    write(fd, &i, 4);  // info header size
    i = *width;
    write(fd, &i, 4);
    i = *height;
    write(fd, &i, 4);
    i = (32 << 16) | 1;
    write(fd, &i, 4);  // planes & bpp
    i = 0;
    write(fd, &i, 4); // compression
    i = *width * *height * 4;
    write(fd, &i, 4); // image size
    i = 0;
    write(fd, &i, 4); // pixels per meter x
    write(fd, &i, 4); // pixels per meter y
    write(fd, &i, 4); // color table size
    write(fd, &i, 4); // color table important

    lseek(fd, (*width * *height * 4) + 4095, SEEK_SET);
    write(fd, "", 1);

    *fb = (uint32_t*)mmap(0, *width * *height * 4, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 4096);

    close(fd);
}
#endif

void hsv_to_rgb (float h, float s, float v, uint8_t *r_r, uint8_t *r_g, uint8_t *r_b) {
    double      hh, p, q, t, ff;
    long        i;

    if(s <= 0.0) {       // < is bogus, just shuts up warnings
        *r_r = v * 255.f;
        *r_g = v * 255.f;
        *r_b = v * 255.f;
    }
    hh = h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = v * (1.0 -  s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));

    switch(i) {
    case 0:
        *r_r = 255.0 * v;
        *r_g = 255.0 * t;
        *r_b = 255.0 * p;
        break;
    case 1:
        *r_r = 255.0 * q;
        *r_g = 255.0 * v;
        *r_b = 255.0 * p;
        break;
    case 2:
        *r_r = 255.0 * p;
        *r_g = 255.0 * v;
        *r_b = 255.0 * t;
        break;
    case 3:
        *r_r = 255.0 * p;
        *r_g = 255.0 * q;
        *r_b = 255.0 * v;
        break;
    case 4:
        *r_r = 255.0 * t;
        *r_g = 255.0 * p;
        *r_b = 255.0 * v;
        break;
    case 5:
    default:
        *r_r = 255.0 * v;
        *r_g = 255.0 * p;
        *r_b = 255.0 * q;
        break;
    }
}

int main(int argc, char* argv[]) {
    uint32_t* fb;
    uint16_t width, height, stride, colorFormat;
    sc_do_hardware_framebuffer(&fb, &width, &height, &stride, &colorFormat);

    std::mutex fb_mutex;
    fb_mutex.lock();

    const uint16_t forks = 4;
    const uint16_t grid  = std::sqrt(forks);

    for(int i = 0; i < forks; ++i) {
        volatile int pid = fork();

        if(pid == 0) {
            const uint16_t row = i % grid;
            const uint16_t col = i / grid;

            const uint16_t xstart = col * (width  / grid);
            const uint16_t ystart = row * (height / grid);

            float h = (row / (float)grid) * 120;
            float s = 0.75;
            float v = 0.5 + (col / ((float)grid) * 0.5);

            uint8_t color[4];
            color[3] = 0;

            while(true) {
                const std::lock_guard<std::mutex> lock(fb_mutex);

                hsv_to_rgb(h, s, v, &color[2], &color[1], &color[0]);

                for(size_t y = 0; y < height / grid; ++y) {
                    for(size_t x = 0; x < width / grid; ++x) {
                        fb[(ystart + y) * width + xstart + x] = *(uint32_t*)color;
                    }
                }

                h += 5;
                while(h > 360) {
                    h -= 360;
                }

#if defined(__linux)
                return EXIT_SUCCESS;
#endif
            }
        }
    }

    fb_mutex.unlock();

    return EXIT_SUCCESS;
}
