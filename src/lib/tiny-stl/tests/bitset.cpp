#include <random>
#include <bitset>

#include <gtest/gtest.h>

#define NAMESPACE_FOR_TESTING
#include "../include/std/bitset"

template<typename T>
class BitsetTest : public testing::Test {};

using BitsetTestTypes = ::testing::Types<
    tinystl_std::bitset<64>,
    std::bitset<64>
>;
TYPED_TEST_SUITE(BitsetTest, BitsetTestTypes);

TYPED_TEST(BitsetTest, Simple) {
    std::mt19937 rng;
    uint64_t random = rng();

    TypeParam foo {};

    for(size_t i = 0; i < 64; ++i) {
        foo.set(i, random & (1ULL << i));
    }

    for(size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(foo.test(i), (random & (1ULL << i)) != 0);
    }

    foo.flip();
    for(size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(foo.test(i), (random & (1ULL << i)) == 0);
    }

    foo.set();
    for(size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(foo.test(i), true);
    }

    foo.reset();
    for(size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(foo.test(i), false);
    }

    EXPECT_FALSE(foo.any());
    EXPECT_FALSE(foo.all());
    EXPECT_TRUE(foo.none());

    foo.flip();
    for(size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(foo.test(i), true);
    }

    EXPECT_TRUE(foo.any());
    EXPECT_TRUE(foo.all());
    EXPECT_FALSE(foo.none());
}
