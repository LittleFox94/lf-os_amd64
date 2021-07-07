#include <lfostest.h>

namespace LFOS {
    #include <message_passing.h>

    #define MT_Invalid     Message::MT_Invalid
    #define MT_UserDefined Message::MT_UserDefined

    extern "C" {
        #include <mq.c>
        #include <tpa.c>
    }

    void scheduler_waitable_done(enum wait_reason r, union wait_data d, size_t m) {
    }

    class MessageQueueTest : public ::testing::Test {
        public:
            MessageQueueTest()
                : _message((Message*)malloc(sizeof(Message) + 1)) {
                init_mq(&kernel_alloc);
                _messageQueue = mq_create(&kernel_alloc);

                _message->size             = 1 + sizeof(struct Message);
                _message->user_size        = 1;
                _message->type             = MT_UserDefined;
                _message->user_data.raw[0] = rand() % 0xFF;

                RecordProperty("RandomMessageContent", _message->user_data.raw[0]);
            }

            ~MessageQueueTest() {
                mq_destroy(_messageQueue);
                kernel_alloc.dealloc(&kernel_alloc, mqs);
            }

        protected:
            uint64_t _messageQueue;
            Message* _message;
    };

    TEST_F(MessageQueueTest, PopFromEmptyQueue) {
        struct Message msg = { .size = sizeof(Message) };
        EXPECT_EQ(mq_peek(_messageQueue, &msg), ENOMSG) << "Did not peek message from empty queue";
    }

    TEST_F(MessageQueueTest, UnlimitedQueue) {
        // TODO: interface for setting queue limits, make unlimited here
        //mq.max_items = 0;
        //mq.max_bytes = 0;

        EXPECT_EQ(mq_push(_messageQueue, _message), 0) << "Added message to unlimited queue";

        Message* msg = (Message*)malloc(sizeof(Message) + 1);
        msg->size = sizeof(msg);

        EXPECT_EQ(mq_peek(_messageQueue, msg), EMSGSIZE)   << "Did not peek messsage because not enough buffer space";
        EXPECT_EQ(msg->size, sizeof(Message) + 1)          << "After peek we know the needed buffer space";
        EXPECT_EQ(msg->type, MT_Invalid)                   << "When message buffer is too small, message type is MT_Invalid";

        memset(msg, 0, sizeof(Message) + 1);
        msg->size = sizeof(Message) + 1;

        EXPECT_EQ(mq_peek(_messageQueue, msg), 0)                    << "Peeked message from queue with a single message in it";
        EXPECT_EQ(msg->user_data.raw[0], _message->user_data.raw[0]) << "Contents of peeked message correct";
        EXPECT_EQ(msg->type, MT_UserDefined)                         << "Peeked message has correct type";

        memset(msg, 0, sizeof(Message) + 1);
        msg->size = sizeof(Message) + 1;

        EXPECT_EQ(mq_peek(_messageQueue, msg), 0)                    << "Again peeked message from queue with a single message in it";
        EXPECT_EQ(msg->user_data.raw[0], _message->user_data.raw[0]) << "Contents of peeked message correct";
        EXPECT_EQ(msg->type, MT_UserDefined)                         << "Peeked message has correct type";

        memset(msg, 0, sizeof(Message) + 1);
        msg->size = sizeof(Message) + 1;

        EXPECT_EQ(mq_pop(_messageQueue, msg), 0)                     << "Message in queue, popped it";
        EXPECT_EQ(msg->user_data.raw[0], _message->user_data.raw[0]) << "Contents of peeked message correct";
        EXPECT_EQ(msg->type, MT_UserDefined)                         << "Peeked message has correct type";

        memset(msg, 0, sizeof(Message) + 1);
        msg->size = sizeof(Message) + 1;

        EXPECT_EQ(mq_pop(_messageQueue, msg), ENOMSG) << "Popping again gives no message but correct error code";
    }

    TEST_F(MessageQueueTest, ManyMessageTest) {
        const size_t num_messages = 16;

        for(size_t i = 0; i < num_messages; ++i) {
            Message* msg = (Message*)malloc(sizeof(Message) + 1);
            msg->size             = sizeof(Message) + 1;
            msg->user_size        = 1;
            msg->user_data.raw[0] = i % 255;

            EXPECT_EQ(mq_push(_messageQueue, msg), 0) << "Pushed message to queue";
        }

        for(size_t i = 0; i < num_messages; ++i) {
            Message* msg_peeked = (Message*)malloc(sizeof(Message) + 1);
            msg_peeked->size = sizeof(Message) + 1;

            Message* msg_popped = (Message*)malloc(sizeof(Message) + 1);
            msg_popped->size = sizeof(Message) + 1;

            EXPECT_EQ(mq_peek(_messageQueue, msg_peeked), 0) << "Successfully peeked message from queue";
            EXPECT_EQ(mq_pop(_messageQueue,  msg_popped), 0) << "Successfully popped message from queue";

            EXPECT_EQ(msg_peeked->size, msg_popped->size)                         << "Messages have identical size";
            EXPECT_EQ(msg_peeked->user_data.raw[0], msg_popped->user_data.raw[0]) << "Messages have identical raw user data";
            EXPECT_EQ(msg_peeked->user_data.raw[0], i)                            << "Messages have correct raw user data";

            free(msg_peeked);
            free(msg_popped);
        }
    }

    // TODO: build new interface for limitting queues and test the functionality here
    /*TEST_F(MessageQueueTest, LimitedQueue) {
        //mq.max_items = 0;
        //mq.max_bytes = message.size;
        //t->eq_bool(false, mq_push(&mq, &message), "Correctly denied adding to item-full queue");
        //t->eq_ptr_t(0,  (ptr_t)mq_peek(&mq),      "No data in queue, by peek");

        //mq.max_items = 1;
        //mq.max_bytes = 0;
        //t->eq_bool(false, mq_push(&mq, &message), "Correctly denied adding to byte-full queue");
        //t->eq_ptr_t(0,   (ptr_t)mq_peek(&mq),     "No data in queue, by peek");
    }*/
}
