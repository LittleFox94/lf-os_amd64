#include <algorithm>
#include <random>
#include <forward_list>
#include <ranges>

#include <gtest/gtest.h>

#define NAMESPACE_FOR_TESTING
#include "../include/std/forward_list"

template<typename T>
class ForwardListTest : public testing::Test {};

struct Box {
    int value;
    Box(int v) : value(v) { }
};

using ForwardListTestTypes = ::testing::Types<
    tinystl_std::forward_list<Box>,
    std::forward_list<Box>
>;
TYPED_TEST_SUITE(ForwardListTest, ForwardListTestTypes);

TYPED_TEST(ForwardListTest, Simple) {
    std::mt19937 rng;

    std::vector<int> values(65);
    std::ranges::generate(
        values,
        [&rng]{
            return rng();
        }
    );

    TypeParam foo {};

    foo.emplace_front(values[0]);

    auto last = foo.begin();
    for(auto it = values.begin() + 1; it != values.end(); ++it) {
        last = foo.emplace_after(last, Box(*it));
    }

    size_t i = 0;
    for(auto it = foo.begin(); it != foo.end(); ++it) {
        EXPECT_EQ(it->value, values[i++]);
    }

    foo.pop_front();
    values.erase(values.begin());
    i = 0;
    for(auto it = foo.begin(); it != foo.end(); ++it) {
        EXPECT_EQ(it->value, values[i++]);
    }

    values.erase(values.begin() + 1);
    foo.erase_after(foo.begin());
    i = 0;
    for(auto it = foo.begin(); it != foo.end(); ++it) {
        EXPECT_EQ(it->value, values[i++]);
    }
}
