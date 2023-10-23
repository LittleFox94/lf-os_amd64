#include <stdlib.h>
#include <gtest/gtest.h>

#include <lfos/dlinkedlist.h>

class dlinkedlistTest : public testing::Test {
    protected:
        void SetUp() override {
            dlinkedlist = lfos_dlinkedlist_init();
        }

        void TearDown() override {
            lfos_dlinkedlist_destroy(dlinkedlist);
        }

        char* getTestString() {
            size_t size = snprintf(nullptr, 0, "test string %d", testStringCounter++) + 1;
            char* s = (char*)calloc(size, 1);
            snprintf(s, size, "test string %d", testStringCounter);
            return s;
        }

        void checkFullListForward(std::vector<char*> expected) {
            lfos_dlinkedlist_iterator* lfos_it = lfos_dlinkedlist_front(dlinkedlist);
            for(auto it = expected.begin(); it != expected.end(); it++) {
                SCOPED_TRACE(*it);

                char* data = (char*)lfos_dlinkedlist_data(lfos_it);
                EXPECT_STREQ(data, *it);

                int error = lfos_dlinkedlist_forward(lfos_it);
                if(it + 1 == expected.end()) {
                    EXPECT_EQ(error, ENOENT);
                } else {
                    EXPECT_EQ(error, 0);
                }
            }

            free(lfos_it);

            lfos_it = lfos_dlinkedlist_front(dlinkedlist);
            for(auto it = expected.begin(); it != expected.end(); it++) {
                SCOPED_TRACE(*it);

                char* data = (char*)lfos_dlinkedlist_data(lfos_it);
                EXPECT_STREQ(data, *it);

                lfos_dlinkedlist_iterator* newIt = lfos_dlinkedlist_next(lfos_it);
                free(lfos_it);
                lfos_it = newIt;

                if(it + 1 == expected.end()) {
                    EXPECT_EQ(lfos_it, nullptr);
                } else {
                    EXPECT_NE(lfos_it, nullptr);
                }
            }

            free(lfos_it);
        }

        void checkFullListBackward(std::vector<char*> expected) {
            lfos_dlinkedlist_iterator* lfos_it = lfos_dlinkedlist_back(dlinkedlist);
            for(auto it = expected.rbegin(); it != expected.rend(); it++) {
                SCOPED_TRACE(*it);

                char* data = (char*)lfos_dlinkedlist_data(lfos_it);
                EXPECT_STREQ(data, *it);

                int error = lfos_dlinkedlist_backward(lfos_it);
                if(it + 1 == expected.rend()) {
                    EXPECT_EQ(error, ENOENT);
                } else {
                    EXPECT_EQ(error, 0);
                }
            }

            free(lfos_it);

            lfos_it = lfos_dlinkedlist_back(dlinkedlist);
            for(auto it = expected.rbegin(); it != expected.rend(); it++) {
                SCOPED_TRACE(*it);

                char* data = (char*)lfos_dlinkedlist_data(lfos_it);
                EXPECT_STREQ(data, *it);

                lfos_dlinkedlist_iterator* newIt = lfos_dlinkedlist_prev(lfos_it);
                free(lfos_it);
                lfos_it = newIt;

                if(it + 1 == expected.rend()) {
                    EXPECT_EQ(lfos_it, nullptr);
                } else {
                    EXPECT_NE(lfos_it, nullptr);
                }
            }

            free(lfos_it);
        }

        int testStringCounter = 0;
        lfos_dlinkedlist* dlinkedlist;
};

TEST_F(dlinkedlistTest, Basic) {
    std::vector<char*> items = {
        getTestString(),
        getTestString(),
        getTestString(),
    };

    lfos_dlinkedlist_iterator* it = lfos_dlinkedlist_back(dlinkedlist);
    EXPECT_EQ(lfos_dlinkedlist_data(it), nullptr) << "there should be no data in the back iterator";
    free(it);

    it = lfos_dlinkedlist_front(dlinkedlist);
    EXPECT_EQ(lfos_dlinkedlist_data(it), nullptr) << "there should be no data in the front iterator";

    lfos_dlinkedlist_insert_after(it, items[0]);
    EXPECT_STREQ((char*)lfos_dlinkedlist_data(it), items[0]) << "data of the front iterator should match our first test item";

    lfos_dlinkedlist_insert_after(it, items[1]);
    EXPECT_EQ(lfos_dlinkedlist_forward(it), 0) << "forwarding our iterator should not return an error";
    EXPECT_STREQ((char*)lfos_dlinkedlist_data(it), items[1]) << "data of the item after the front iterator should be our second test item";

    lfos_dlinkedlist_insert_after(it, items[2]);
    EXPECT_EQ(lfos_dlinkedlist_forward(it), 0) << "forwarding our iterator should not return an error";
    EXPECT_STREQ((char*)lfos_dlinkedlist_data(it), items[2]) << "data of the item two after the front iterator should now be our third test item";

    lfos_dlinkedlist_iterator* front = lfos_dlinkedlist_front(dlinkedlist);
    lfos_dlinkedlist_iterator* second = lfos_dlinkedlist_next(front);
    EXPECT_NE(second, nullptr) << "we should have an iterator after front";

    lfos_dlinkedlist_iterator* third = lfos_dlinkedlist_next(second);
    EXPECT_NE(third, nullptr) << "we should have an iterator after second";

    EXPECT_STREQ((char*)lfos_dlinkedlist_data(second), items[1]) << "data of the item one after the front iterator should now be our second test item";
    EXPECT_STREQ((char*)lfos_dlinkedlist_data(third), items[2]) << "data of the item two after the front iterator should now be our third test item";

    free(third);
    free(second);
    free(front);
    free(it);
}

TEST_F(dlinkedlistTest, MiddleInsert) {
    std::vector<char*> items = {
        getTestString(),
        getTestString(),
        getTestString(),
        getTestString(),
    };

    lfos_dlinkedlist_iterator* it = lfos_dlinkedlist_front(dlinkedlist);

    lfos_dlinkedlist_insert_after(it, items[0]);

    lfos_dlinkedlist_insert_after(it, items[3]);
    EXPECT_EQ(lfos_dlinkedlist_forward(it), 0) << "forwarding our iterator should not return an error";

    lfos_dlinkedlist_insert_before(it, items[1]);
    EXPECT_EQ(lfos_dlinkedlist_backward(it), 0) << "backwarding our iterator should not return an error";
    EXPECT_STREQ((char*)lfos_dlinkedlist_data(it), items[1]) << "data of the item two after the front iterator should now be our third test item";

    lfos_dlinkedlist_insert_after(it, items[2]);
    EXPECT_EQ(lfos_dlinkedlist_forward(it), 0) << "forwarding our iterator should not return an error";
    EXPECT_STREQ((char*)lfos_dlinkedlist_data(it), items[2]) << "data of the item two after the front iterator should now be our third test item";

    free(it);

    checkFullListForward(items);
    checkFullListBackward(items);
}

TEST_F(dlinkedlistTest, MiddleRemove) {
    std::vector<char*> items = {
        getTestString(),
        getTestString(),
        getTestString(),
        getTestString(),
    };

    lfos_dlinkedlist_iterator* it = lfos_dlinkedlist_front(dlinkedlist);

    lfos_dlinkedlist_insert_after(it, items[0]);

    lfos_dlinkedlist_insert_after(it, items[3]);
    EXPECT_EQ(lfos_dlinkedlist_forward(it), 0) << "forwarding our iterator should not return an error";

    lfos_dlinkedlist_insert_before(it, items[1]);
    EXPECT_EQ(lfos_dlinkedlist_backward(it), 0) << "backwarding our iterator should not return an error";
    EXPECT_STREQ((char*)lfos_dlinkedlist_data(it), items[1]) << "data of the item two after the front iterator should now be our third test item";

    lfos_dlinkedlist_insert_after(it, getTestString());
    EXPECT_EQ(lfos_dlinkedlist_forward(it), 0) << "forwarding our iterator should not return an error";
    EXPECT_EQ(lfos_dlinkedlist_unlink(it), 0) << "deleting our entry should not return an error";
    EXPECT_EQ(lfos_dlinkedlist_forward(it), 0) << "forwarding our iterator should not return an error";
    EXPECT_EQ(lfos_dlinkedlist_backward(it), 0) << "forwarding our iterator should not return an error";

    lfos_dlinkedlist_insert_after(it, items[2]);
    EXPECT_EQ(lfos_dlinkedlist_forward(it), 0) << "forwarding our iterator should not return an error";
    EXPECT_STREQ((char*)lfos_dlinkedlist_data(it), items[2]) << "data of the item two after the front iterator should now be our third test item";

    free(it);

    checkFullListForward(items);
    checkFullListBackward(items);
}

TEST_F(dlinkedlistTest, IteratingAndInserting) {
    std::vector<char*> items;
    int error = 0;

    lfos_dlinkedlist_iterator* it;

    {
        it = lfos_dlinkedlist_front(dlinkedlist);

        SCOPED_TRACE("adding entries while iterating forwards after each insertion, starting at front");

        for(size_t i = 0; i < 10; ++i) {
            items.push_back(getTestString());

            error = lfos_dlinkedlist_insert_after(it, items[i]);
            EXPECT_EQ(error, 0) << "inserting our new item should succeed";

            error = lfos_dlinkedlist_forward(it);

            if(i) {
                EXPECT_EQ(error, 0) << "forwarding our iterator after inserting our new item should succeed with i > 0";
            } else {
                EXPECT_EQ(error, ENOENT) << "forwarding our iterator after inserting our new item should fail with ENOENT for i == 0";
            }
        }
    }

    {
        SCOPED_TRACE("iterating backwards from the iterator after last insertion, checking for correct data being stored");

        for(ssize_t i = items.size() - 1; i >= 0; --i) {
            char* data = (char*)lfos_dlinkedlist_data(it);
            EXPECT_STREQ(data, items[i]) << "should return the data we stored before";

            error = lfos_dlinkedlist_backward(it);

            if(i) {
                EXPECT_EQ(error, 0) << "backwarding (is that even a word?) our iterator after retrieving our item should succeed with i > 0";
            } else {
                EXPECT_EQ(error, ENOENT) << "backwarding our iterator before the first item should fail with ENOENT";
            }
        }

        free(it);
    }

    {
        SCOPED_TRACE("adding and then removing an entry before first element");

        it = lfos_dlinkedlist_front(dlinkedlist);

        char* item = getTestString();
        error = lfos_dlinkedlist_insert_before(it, item);
        EXPECT_EQ(error, 0) << "inserting item before first element should succeed";

        error = lfos_dlinkedlist_backward(it);
        EXPECT_EQ(error, 0) << "iterating backward should succeed";

        EXPECT_STREQ((char*)lfos_dlinkedlist_data(it), item) << "stored data should match what we inserted";

        error = lfos_dlinkedlist_unlink(it);
        EXPECT_EQ(error, 0) << "unlinking item should succeed";

        error = lfos_dlinkedlist_forward(it);
        EXPECT_EQ(error, 0) << "iterating forward from iterator to unlinked item should succeed";

        EXPECT_STREQ((char*)lfos_dlinkedlist_data(it), items[0]) << "stored data should match our first test item";

        free(it);
    }

    checkFullListForward(items);
    checkFullListBackward(items);
}

TEST_F(dlinkedlistTest, InsertingBackwards) {
    std::vector<char*> items;
    for(size_t i = 0; i < 10; ++i) {
        items.push_back(getTestString());
    }

    int error = 0;

    lfos_dlinkedlist_iterator* it;

    {
        it = lfos_dlinkedlist_back(dlinkedlist);

        SCOPED_TRACE("adding entries while iterating backwards after each insertion, starting at back");

        for(auto i = items.rbegin(); i != items.rend(); i++) {
            error = lfos_dlinkedlist_insert_before(it, *i);
            EXPECT_EQ(error, 0) << "inserting our new item should succeed";

            error = lfos_dlinkedlist_backward(it);

            if(i != items.rbegin()) {
                EXPECT_EQ(error, 0) << "backwarding our iterator after inserting our new item should succeed with i > 0";
            } else {
                EXPECT_EQ(error, ENOENT) << "backwarding our iterator after inserting our new item should fail with ENOENT for i == 0";
            }
        }

        free(it);
    }

    checkFullListForward(items);
    checkFullListBackward(items);
}
