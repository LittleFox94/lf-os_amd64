#include <lfostest.h>

namespace LFOS {
    #define __kernel 1

    extern "C" {
        #include <message_passing.h>
        #include "../cstdlib/string.h"

        #include "../sd.c"
        #include "../mq.c"
        #include "../tpa.c"
        #include "../uuid.c"
    }

    void scheduler_waitable_done(enum wait_reason r, union wait_data d, size_t m) {
    }

    TEST(KernelServiceDiscovery, Basic) {
        init_mq(&kernel_alloc);

        mq_id_t mq0 = mq_create(&kernel_alloc);
        mq_id_t mq1 = mq_create(&kernel_alloc);

        init_sd();

        uuid_t a  = { { 0x1234a27c, 0xdc50, 0x4e38, 0x8c2b, { 0x45, 0x72, 0xd2, 0xe9, 0x2b, 0x80 } } };
        uuid_t a2 = { { 0x1234a27c, 0xdc50, 0x4e38, 0x8c2b, { 0x45, 0x72, 0xd2, 0xe9, 0x2a, 0x81 } } };
        uuid_t b  = { { 0x8bf252c9, 0x4227, 0x472c, 0x963f, { 0xd7, 0x42, 0xf9, 0x77, 0xac, 0x41 } } };

        ASSERT_EQ(uuid_key(&a), uuid_key(&a2)) << "Collision check UUIDs have same key";

        sd_register(&a,  mq0);
        sd_register(&b,  mq0);
        sd_register(&b,  mq1);
        sd_register(&a2, mq1);

        struct Message msgA = {
            .size   = sizeof(Message),
            .sender = 0x1337A,
            .type   = MT_ServiceDiscovery,
        };

        EXPECT_EQ(sd_send(&a, &msgA), 1) << "Message sent to registered queue for a";

        struct Message msgB = {
            .size   = sizeof(Message),
            .sender = 0x1337B,
            .type   = MT_ServiceDiscovery,
        };

        EXPECT_EQ(sd_send(&b, &msgB), 2) << "Message sent to all two reqistered queues for b";

        struct Message msgA2 = {
            .size   = sizeof(Message),
            .sender = 0x1337A2,
            .type   = MT_ServiceDiscovery,
        };

        EXPECT_EQ(sd_send(&a2, &msgA2), 1) << "Message sent to reqistered queue for a2";

        struct Message msgRecv = {
            .size = sizeof(Message),
        };

        EXPECT_EQ(mq_pop(mq0, &msgRecv), 0) << "Received message on queue 0";
        EXPECT_EQ(msgRecv.sender, 0x1337A)  << "Successfully received message for service a on queue 0";

        EXPECT_EQ(mq_pop(mq0, &msgRecv), 0) << "Received message on queue 0";
        EXPECT_EQ(msgRecv.sender, 0x1337B)  << "Successfully received message for service b on queue 0";

        EXPECT_EQ(mq_pop(mq1, &msgRecv), 0) << "Received message on queue 1";
        EXPECT_EQ(msgRecv.sender, 0x1337B)  << "Successfully received message for service b on queue 1";

        EXPECT_EQ(mq_pop(mq1, &msgRecv), 0) << "Received message on queue 1";
        EXPECT_EQ(msgRecv.sender, 0x1337A2) << "Successfully received message for service a2 on queue 1";

        EXPECT_EQ(mq_pop(mq0, 0), ENOMSG) << "No more messages on queue 0, as expected";
        EXPECT_EQ(mq_pop(mq1, 0), ENOMSG) << "No more messages on queue 1, as expected";

        mq_destroy(mq0);
        mq_destroy(mq1);
    }
}
