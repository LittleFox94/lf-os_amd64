#include "lfostest.h"

#define __kernel 1
#include "../sd.c"
#include "../mq.c"
#include "../tpa.c"
#include "../uuid.c"

void scheduler_waitable_done(enum wait_reason r, union wait_data d, size_t m) {
}

__attribute__ ((visibility ("default"))) void testmain(TestUtils* t) {
    init_mq(&kernel_alloc);

    mq_id_t mq0 = mq_create(&kernel_alloc);
    mq_id_t mq1 = mq_create(&kernel_alloc);

    init_sd();

    uuid_t a  = { { 0x1234a27c, 0xdc50, 0x4e38, 0x8c2b, { 0x45, 0x72, 0xd2, 0xe9, 0x2b, 0x80 } } };
    uuid_t a2 = { { 0x1234a27c, 0xdc50, 0x4e38, 0x8c2b, { 0x45, 0x72, 0xd2, 0xe9, 0x2a, 0x81 } } };
    uuid_t b  = { { 0x8bf252c9, 0x4227, 0x472c, 0x963f, { 0xd7, 0x42, 0xf9, 0x77, 0xac, 0x41 } } };

    t->eq_uint8_t(uuid_key(&a), uuid_key(&a2), "Collision check UUIDs have same key");

    sd_register(&a,  mq0);
    sd_register(&b,  mq0);
    sd_register(&b,  mq1);
    sd_register(&a2, mq1);

    struct Message msgA = {
        .sender = 0x1337A,
        .size   = sizeof(struct Message),
        .type   = MT_ServiceDiscovery,
    };

    t->eq_uint64_t(1, sd_send(&a, &msgA), "Message sent to registered queue for a");

    struct Message msgB = {
        .sender = 0x1337B,
        .size   = sizeof(struct Message),
        .type   = MT_ServiceDiscovery,
    };

    t->eq_uint64_t(2, sd_send(&b, &msgB), "Message sent to all two reqistered queues for b");

    struct Message msgA2 = {
        .sender = 0x1337A2,
        .size   = sizeof(struct Message),
        .type   = MT_ServiceDiscovery,
    };

    t->eq_uint64_t(1, sd_send(&a2, &msgA2), "Message sent to reqistered queue for a2");

    struct Message msgRecv = {
        .size = sizeof(struct Message),
    };

    t->eq_uint64_t(0, mq_pop(mq0, &msgRecv), "Received message on queue 0");
    t->eq_uint64_t(0x1337A, msgRecv.sender, "Successfully received message for service a on queue 0");

    t->eq_uint64_t(0, mq_pop(mq0, &msgRecv), "Received message on queue 0");
    t->eq_uint64_t(0x1337B, msgRecv.sender, "Successfully received message for service b on queue 0");

    t->eq_uint64_t(0, mq_pop(mq1, &msgRecv), "Received message on queue 1");
    t->eq_uint64_t(0x1337B, msgRecv.sender, "Successfully received message for service b on queue 1");

    t->eq_uint64_t(0, mq_pop(mq1, &msgRecv), "Received message on queue 1");
    t->eq_uint64_t(0x1337A2, msgRecv.sender, "Successfully received message for service a2 on queue 1");

    t->eq_uint64_t(ENOMSG, mq_pop(mq0, 0), "No more messages on queue 0, as expected");
    t->eq_uint64_t(ENOMSG, mq_pop(mq1, 0), "No more messages on queue 1, as expected");

    mq_destroy(mq0);
    mq_destroy(mq1);
}
