#define _TESTRUNNER

#include <iostream>
#include <dlfcn.h>
#include <stdarg.h>

#include <gtest/gtest.h>

#define LFOS_API __attribute__((visibility("default")))

extern "C" {
    #include "lfostest.h"
    #include "../allocator.h"

    LFOS_API allocator_t kernel_alloc = allocator_t {
        .alloc   = [](allocator_t* alloc, size_t size) -> void* {
            alloc->tag += size;
            uint8_t* data = new uint8_t[size + sizeof(size_t)];
            (reinterpret_cast<size_t*>(data))[0] = size;
            return data+sizeof(size_t);
        },
        .dealloc = [](allocator_t* alloc, void* ptr) -> void {
            size_t* data = reinterpret_cast<size_t*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(size_t));
            alloc->tag -= *data;
            delete[] data;
        }
    };

    LFOS_API void panic_message(const char* message) {
        fprintf(stderr, "Panic() called: %s\n", message);
        exit(-1);
    }

    LFOS_API void panic() {
        panic_message("Unknown error");
    }

    LFOS_API int ksnprintf(char* buffer, int buffer_size, const char* format, ...) {
        va_list args;
        va_start(args, format);
        int count = vsnprintf(buffer, buffer_size, format, args);
        va_end(args);

        return count;
    }
}

int main(int argc, char* argv[]) {
    dlopen(NULL, RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL);

    testing::InitGoogleTest(&argc, argv);

    for(int i = 1; i < argc; ++i) {
        if(!dlopen(argv[i], RTLD_NOW)) {
            char* error = dlerror();

            if(!error) {
                error = (char*)"Unknown error";
            }

            std::cerr << "Could not load testlib \"" << argv[i] << "\": " << error << std::endl;
            abort();
        }
    }

    return RUN_ALL_TESTS();
}
