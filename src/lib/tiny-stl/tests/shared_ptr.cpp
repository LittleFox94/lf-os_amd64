#include <random>
#include <bitset>

#include <gtest/gtest.h>

#define NAMESPACE_FOR_TESTING
#include "../include/std/memory"

struct TestObject {
    int val = 42;

    TestObject(bool* deleted) :
        _deleted(deleted) { }

    ~TestObject() {
        *_deleted = true;
    }

    private:
        bool* _deleted;
};

template<typename T>
struct SharedPtrTest : public testing::Test {
};

using SharedPtrTestTypes = ::testing::Types<
    tinystl_std::shared_ptr<TestObject>,
    std::shared_ptr<TestObject>
>;
TYPED_TEST_SUITE(SharedPtrTest, SharedPtrTestTypes);

TYPED_TEST(SharedPtrTest, Simple) {
    bool deleted = false;

    {
        TypeParam a(new TestObject(&deleted));

        EXPECT_EQ(a->val, 42);
        ++a->val;
        EXPECT_EQ(a->val, 43);

        EXPECT_EQ(a.use_count(), 1);

        {
            TypeParam b = a;
            EXPECT_EQ(a.use_count(), 2);
            EXPECT_EQ(b.use_count(), 2);

            EXPECT_EQ(b->val, 43);
            ++b->val;
            EXPECT_EQ(a->val, 44);
            EXPECT_EQ(b->val, 44);
        }

        EXPECT_EQ(a.use_count(), 1);

        EXPECT_FALSE(deleted);
    }

    EXPECT_TRUE(deleted);
}
