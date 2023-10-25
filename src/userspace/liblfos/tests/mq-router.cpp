#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <lfos/mq-router.h>

class mqRouterTest : public testing::Test {
    public:
        static std::vector<mqRouterTest*> instances;

        MOCK_METHOD(void, mq_poll, (uint64_t queue, bool block, struct Message* msg, uint64_t* error));

    protected:
        struct messagePollResult {
            uint64_t error;
            struct Message* msg;
        };

        virtual void SetUp() override {
            instances.push_back(this);
            router = lfos_mq_router_init(instances.size() - 1);
        }

        virtual void TearDown() override {
            EXPECT_EQ(lfos_mq_router_destroy(router), 0) << "expect lfos_mq_router_destroy to succeed";
        }


        static int static_message_handler(struct Message* msg) {
            mqRouterTest* fixture = mqRouterTest::instances[msg->sender];
            return fixture->message_handler(msg);
        }

        MOCK_METHOD(int, message_handler, (struct Message*));

        lfos_mq_router* router;

        void queueMessage(uint64_t queuedError, std::optional<std::function<void(struct Message*)>> handler = std::optional<std::function<void(struct Message*)>>()) {
            EXPECT_CALL(*this, mq_poll)
                .InSequence(messageQueueSequence)
                .WillOnce(
                    [handler, queuedError](uint64_t queue, bool block, struct Message* msg, uint64_t* error) {
                        if(handler) {
                            handler.value()(msg);
                        }

                        msg->sender = queue;
                        *error = queuedError;
                    }
                );
        }

    private:
        testing::Sequence messageQueueSequence;
};

std::vector<mqRouterTest*> mqRouterTest::instances;

extern "C" {
    void sc_do_ipc_mq_poll(uint64_t queue, bool block, struct Message* msg, uint64_t* error) {
        mqRouterTest* fixture = mqRouterTest::instances[queue];
        fixture->mq_poll(queue, block, msg, error);
    }
}

MATCHER_P(MessageType, type, "") {
    return std::get<0>(arg)->type == type;
}

MATCHER_P(MessageSize, size, "") {
    return std::get<0>(arg)->size == size;
}

TEST_F(mqRouterTest, NotBlocking) {
    queueMessage(EAGAIN);
    lfos_mq_router_receive_messages(router);
}

TEST_F(mqRouterTest, HandlingMessageSize) {
    queueMessage(EMSGSIZE, [](struct Message* msg) {
        msg->size = sizeof(struct Message) + 42;
    });

    queueMessage(0, [](struct Message* msg) {
        EXPECT_EQ(msg->size, sizeof(struct Message) + 42);
        msg->type = MT_UserDefined;
    });

    queueMessage(EAGAIN);

    EXPECT_CALL(*this, message_handler)
        .With(testing::AllOf(MessageType(MT_UserDefined), MessageSize(sizeof(struct Message) + 42)))
        .WillOnce(testing::Return(0));

    lfos_mq_router_set_handler(router, MT_UserDefined, mqRouterTest::static_message_handler);
    lfos_mq_router_receive_messages(router);
}

TEST_F(mqRouterTest, DeliveringAfterRegistration) {
    queueMessage(0, [](struct Message* msg) {
        msg->type = MT_UserDefined;
    });

    queueMessage(EAGAIN);

    EXPECT_CALL(*this, message_handler)
        .With(MessageType(MT_UserDefined))
        .WillOnce(testing::Return(0));

    lfos_mq_router_receive_messages(router);

    lfos_mq_router_set_handler(router, MT_UserDefined, mqRouterTest::static_message_handler);
}

TEST_F(mqRouterTest, RoutingDifferentTypes) {
    testing::InSequence s;

    auto signalMessageGenerator = [](struct Message* msg) { msg->type = MT_Signal; };
    auto ioMessageGenerator     = [](struct Message* msg) { msg->type = MT_IO; };

    queueMessage(0, signalMessageGenerator);
    queueMessage(0, signalMessageGenerator);
    queueMessage(0, signalMessageGenerator);
    queueMessage(0, ioMessageGenerator);
    queueMessage(0, ioMessageGenerator);
    queueMessage(0, signalMessageGenerator);
    queueMessage(EAGAIN);

    testing::Expectation signalMessages = EXPECT_CALL(*this, message_handler)
        .With(MessageType(MT_Signal))
        .Times(testing::Exactly(4))
        .WillRepeatedly(testing::Return(0));

    EXPECT_CALL(*this, message_handler)
        .With(MessageType(MT_IO))
        .Times(testing::Exactly(2))
        .After(signalMessages)
        .WillRepeatedly(testing::Return(0));

    int error = lfos_mq_router_receive_messages(router);
    EXPECT_EQ(error, 0) << "receiving messages should succeed";

    lfos_mq_router_set_handler(router, MT_Signal, mqRouterTest::static_message_handler);
    lfos_mq_router_set_handler(router, MT_IO, mqRouterTest::static_message_handler);
}
