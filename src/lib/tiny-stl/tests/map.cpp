#include <algorithm>
#include <cstdint>
#include <gtest/gtest.h>
#include <random>
#include <ranges>

#define NAMESPACE_FOR_TESTING
#include "../include/std/map"

template<typename T>
class MapTest : public testing::Test {};

using MapTestTypes = ::testing::Types<
    tinystl_std::map<int, bool>,
    std::map<int, bool>
>;
TYPED_TEST_SUITE(MapTest, MapTestTypes);

TYPED_TEST(MapTest, ReadingConstructedMap) {
    TypeParam foo {
        { 69,     true  },

        { 21,     false },
        { 23,     true  },
        { 42,     true  },
        { 1337,   false },
        { 0x1F05, true },
    };

    EXPECT_TRUE(foo.contains(21));
    EXPECT_TRUE(foo.contains(23));
    EXPECT_TRUE(foo.contains(42));
    EXPECT_TRUE(foo.contains(69));
    EXPECT_TRUE(foo.contains(1337));
    EXPECT_TRUE(foo.contains(0x1F05));

    EXPECT_EQ(foo[21],     false);
    EXPECT_EQ(foo[23],     true);
    EXPECT_EQ(foo[42],     true);
    EXPECT_EQ(foo[69],     true);
    EXPECT_EQ(foo[1337],   false);
    EXPECT_EQ(foo[0x1F05], true);
}

TYPED_TEST(MapTest, ConstructingLargeRandomMap) {
    std::mt19937 rng;

    std::vector<std::pair<int, bool>> values(100000);
    std::ranges::generate(
        values,
        [&rng]{
            int x = rng();
            return std::pair<int, bool>{x, x % 2 == 0};
        }
    );

    TypeParam foo;

    std::ranges::for_each(values, [&foo](const auto& x){ foo.insert(x); });

    EXPECT_EQ(foo.size(), values.size());

    std::ranges::for_each(values, [&foo](const auto& x){
        EXPECT_TRUE(foo.contains(x.first));
        EXPECT_EQ(foo[x.first], x.second);
    });
}

// map used to store a reference to the std::pair given to insert, leading to
// data being silently replaced in a usage like this.
TYPED_TEST(MapTest, Mutate) {
    TypeParam foo;
    for(uint64_t i = 0; i < 5; ++i) {
        foo.insert(std::pair<uint64_t, bool>(i, true));
    }

    EXPECT_EQ(foo.size(), 5);

    for(uint64_t i = 0; i < 5; ++i) {
        EXPECT_TRUE(foo.contains(i));
        EXPECT_EQ(foo[i], true);
    }
}
