#include <lfostest.h>

#include <gtest/gtest.h>

namespace LFOS {
#include <string.c>

    TEST(KernelString, strlen) {
        EXPECT_EQ(strlen("Hello, world!"), 13) << "simple strlen correct";
        EXPECT_EQ(strlen("\t\t"), 2)           << "control characters strlen correct";
        EXPECT_EQ(strlen("\0"), 0)             << "double empty strlen correct";
        EXPECT_EQ(strlen(0), 0)                << "null pointer strlen correct";
    }

    TEST(KernelString, strcmp) {
        EXPECT_LT(strcmp("bar", "foo"), 0) << "strcmp returns negative correctly";
        EXPECT_GT(strcmp("foo", "bar"), 0) << "strcmp returns positive correctly";
        EXPECT_EQ(strcmp("foo", "foo"), 0) << "strcmp returns zero correctly";
    }

    TEST(KernelString, strcasecmp) {
        EXPECT_LT(strcasecmp("bar", "foo"), 0) << "strcasecmp returns negative correctly";
        EXPECT_GT(strcasecmp("foo", "bar"), 0) << "strcasecmp returns positive correctly";
        EXPECT_EQ(strcasecmp("foo", "foo"), 0) << "strcasecmp returns zero correctly";
        EXPECT_EQ(strcasecmp("FoO", "foo"), 0) << "strcasecmp returns zero correctly";
    }

    class SputFamilyTest : public ::testing::Test {
        public:
            SputFamilyTest() {
                memset(_buffer, 0, sizeof(_buffer));
            }

        protected:
            char _buffer[20];
    };

    TEST_F(SputFamilyTest, sputs) {
        EXPECT_EQ(sputs(_buffer, 20, "Hello, world!", 13), 13) << "sputs return correct";
        EXPECT_STREQ(_buffer, "Hello, world!")                 << "sputs modified buffer correctly";
    }

    TEST_F(SputFamilyTest, sputi) {
        EXPECT_EQ(sputi(_buffer, 20, 42, 10), 2) << "sputi return correct";
        EXPECT_STREQ(_buffer, "42")              << "sputi modified buffer correctly";
    }

    TEST_F(SputFamilyTest, sputui) {
        EXPECT_EQ(sputui(_buffer, 20, 23, 10), 2) << "sputui return correct";
        EXPECT_STREQ(_buffer, "23")               << "sputui modified buffer correctly";
    }
       
    TEST_F(SputFamilyTest, sputbytes) {
        EXPECT_EQ(sputbytes(_buffer, 20, 4096), 4) << "sputbytes return correct";
        EXPECT_STREQ(_buffer, "4KiB")             << "sputbytes modified buffer correctly";
    }
}
