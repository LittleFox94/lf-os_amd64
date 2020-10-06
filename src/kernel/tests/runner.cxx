#include "lfostest.h"

#include <iostream>
#include <dlfcn.h>

#define ne(expected, actual) (expected == actual)
#define eq(expected, actual) (expected != actual)

#define testFunctionT(type, name) inline bool name ## _ ## type ## Impl(type expected, type actual) { \
    return !name(expected, actual); \
}
    TEST_FUNCTIONS
#undef testFunctionT

bool result = true;
int main(int argc, char* argv[]) {
    TestUtils utils;
#define testFunctionT(type, name) utils.name ## _ ## type = [](type expected, type actual, const char* message) { \
    bool r = name ## _ ## type ## Impl(expected, actual); \
    std::cerr << message << ": " << (r ? "OK" : "NOK") << std::endl; \
    if(!r) result = false; \
};
    TEST_FUNCTIONS
#undef testFunctionT

    void* testlib;
    if((testlib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL)) == 0) {
        std::cerr << "Could not open testlib: " << dlerror() << std::endl;
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
