#define _TESTRUNNER
#include "lfostest.h"
#include "../allocator.h"

#include <iostream>
#include <dlfcn.h>

#define LFOS_API extern "C" __attribute__((visibility("default")))

#define ne(expected, actual) (expected != actual)
#define eq(expected, actual) (expected == actual)
#define lt(expected, actual) (expected >  actual)
#define gt(expected, actual) (expected <  actual)

#define testFunctionT(type, name) inline bool name ## _ ## type ## Impl(type expected, type actual) { \
    return name(expected, actual); \
}
    TEST_FUNCTIONS
#undef testFunctionT

LFOS_API allocator_t* alloc = 0;

bool result = true;
int main(int argc, char* argv[]) {
    alloc = new allocator_t {
        .alloc   = [](allocator_t* alloc, size_t size) -> void* {
            alloc->tag += size + 8;
            uint8_t* data = new uint8_t[size + sizeof(size_t)];
            (reinterpret_cast<size_t*>(data))[0] = size;
            return data+sizeof(size_t);
        },
        .dealloc = [](allocator_t* alloc, void* ptr) -> void {
            size_t* data = reinterpret_cast<size_t*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(size_t));
            alloc->tag -= *data;
            delete data;
        }
    };

    TestUtils utils;
#define testFunctionT(type, name) utils.name ## _ ## type = [](type expected, type actual, const char* message) { \
    bool r = name ## _ ## type ## Impl(expected, actual); \
    std::cerr << "\e[38;5;" << (r ? "2mok " : "1mnok") << "  " << message << " (" << std::hex << expected << " " #name " " << std::hex << actual << ")" << std::endl; \
    if(!r) result = false; \
};
    TEST_FUNCTIONS
#undef testFunctionT

    dlopen(NULL, RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL);

    void* testlib;
    if((testlib = dlopen(argv[1], RTLD_NOW)) == 0) {
        std::cerr << "Could not load testlib: " << dlerror() << std::endl;
        return -1;
    }

    TestMain testMain = (TestMain)dlsym(testlib, "testmain");
    char* error;
    if((error = dlerror())) {
        std::cerr << "Could not find testmain in " << argv[0] << ": " << error << std::endl;
        return -1;
    }

    testMain(&utils);
    dlclose(testlib);

    return result ? 0 : 1;
}

LFOS_API void panic_message(const char* message) {
    fprintf(stderr, "Panic() called: %s\n", message);
    exit(-1);
}

LFOS_API void panic() {
    panic_message("Unknown error");
}
