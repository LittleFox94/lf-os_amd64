#include <gtest/gtest.h>

TEST(HelloWorld, Hello) {
    volatile int foo = 1000000;
    while(--foo > 0);

    EXPECT_EQ(foo, 0);
}
