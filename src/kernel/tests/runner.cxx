#define _TESTRUNNER

#include <iostream>
#include <dlfcn.h>
#include <stdarg.h>

#include <gtest/gtest.h>

#define LFOS_API __attribute__((visibility("default")))

#define ne(expected, actual, msg) EXPECT_NE(actual, expected) << msg
#define eq(expected, actual, msg) EXPECT_EQ(actual, expected) << msg
#define lt(expected, actual, msg) EXPECT_LT(actual, expected) << msg
#define gt(expected, actual, msg) EXPECT_GT(actual, expected) << msg

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

class LFOSTestFixture : public testing::Test {
    public:
        LFOSTestFixture() {
#define testFunctionT(type, name) _utils.name ## _ ## type = [](type expected, type actual, const char* message) -> void { \
    name(expected, actual, message); \
};
    TEST_FUNCTIONS
#undef testFunctionT

        }

    protected:
        TestUtils _utils;
};

class LFOSTest : public LFOSTestFixture {
    public:
        explicit LFOSTest(const char* libPath)
            : _libPath(libPath) {

            char* error = 0;

            if((_lib = dlopen(_libPath, RTLD_NOW)) == 0) {
                error = dlerror();

                if(!error) {
                    error = (char*)"Unknown error";
                }

                std::cerr << "Could not load testlib \"" << _libPath << "\": " << error << std::endl;
                abort();
            }

            _testMain = (TestMain)dlsym(_lib, "testmain");
            if((error = dlerror())) {
                std::cerr << "Could not find symbol \"testmain\" in \"" << _libPath << "\": " << error << std::endl;
                abort();
            }

            const char** testNamePtr = (const char**)dlsym(_lib, "TestName");

            if(testNamePtr) {
                _testName = *testNamePtr;
            }
        }

        const char* name() const {
            return _testName;
        }

        void TestBody() override {
            _testMain(&_utils);
        }

        void TearDown() override {
            dlclose(_lib);
        }

    private:
        const char* _libPath;
        void*       _lib;
        TestMain    _testMain;
        const char* _testName;
};

int main(int argc, char* argv[]) {
    TestUtils utils;

    dlopen(NULL, RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL);

    testing::InitGoogleTest(&argc, argv);

    for(int i = 1; i < argc; ++i) {
        LFOSTest* test = new LFOSTest(argv[i]);

        if(!test->name()) {
            // C++ test with GTest, registers on its own
            test->TestBody();
            delete test;
        }
        else {
            testing::RegisterTest(
                test->name(), "TestMain", nullptr, nullptr,
                __FILE__, __LINE__,
                [=]() -> LFOSTestFixture* { return test; }
            );
        }
    }

    return RUN_ALL_TESTS();
}
