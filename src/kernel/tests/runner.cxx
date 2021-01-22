#define _TESTRUNNER
#include "lfostest.h"

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

bool result = true;
int main(int argc, char* argv[]) {
    TestUtils utils;
#define testFunctionT(type, name) utils.name ## _ ## type = [](type expected, type actual, const char* message) { \
    bool r = name ## _ ## type ## Impl(expected, actual); \
    std::cerr << "\e[38;5;" << (r ? "2mok " : "1mnok") << "  " << message << " (" << (uint64_t)expected << " " #name " " << (uint64_t)actual << ")" << std::endl; \
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
