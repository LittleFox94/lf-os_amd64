#include <lfostest.h>
#include <mq.c>
#include <tpa.c>

void scheduler_waitable_done(enum wait_reason r, union wait_data d, size_t m) {
}

__attribute__ ((visibility ("default"))) void testmain(TestUtils* t) {
    init_mq(malloc, free);

    uint64_t mq = mq_create(malloc, free);

    struct Message message;
    message.size             = 1 + sizeof(struct Message);
    message.user_size        = 1;
    message.type             = MT_UserDefined;
    message.user_data.raw[0] = 0xa;

    // TODO: new interface for that
    //mq.max_items = 0;
    //mq.max_bytes = message.size;
    //t->eq_bool(false, mq_push(&mq, &message), "Correctly denied adding to item-full queue");
    //t->eq_ptr_t(0,  (ptr_t)mq_peek(&mq),      "No data in queue, by peek");

    //mq.max_items = 1;
    //mq.max_bytes = 0;
    //t->eq_bool(false, mq_push(&mq, &message), "Correctly denied adding to byte-full queue");
    //t->eq_ptr_t(0,   (ptr_t)mq_peek(&mq),     "No data in queue, by peek");

    struct Message msg = { .size = sizeof(struct Message) - 1 };
    t->eq_uint64_t(ENOMSG, mq_peek(mq, &msg), "Did not peek message from empty queue");

    //mq.max_items = 0;
    //mq.max_bytes = 0;
    t->eq_uint64_t(0, mq_push(mq, &message), "Added message to unlimited queue");

    t->eq_uint64_t(EMSGSIZE, mq_peek(mq, &msg), "Did not peek messsage because not enough buffer space");
    t->eq_uint64_t(sizeof(struct Message) + 1, msg.size, "After peek we know the needed buffer space");

    msg.size = sizeof(struct Message) + 1;
    t->eq_uint64_t(0, mq_peek(mq, &msg), "Peeked message from queue with a single message in it");
    t->eq_uint64_t(0, mq_peek(mq, &msg), "Again peeked message from queue with a single message in it");

    t->eq_uint64_t(0, mq_pop(mq, &msg), "Message in queue, popped it");

    const size_t num_messages = 16;
    for(size_t i = 0; i < num_messages; ++i) {
        message.size             = sizeof(struct Message) + 1;
        message.user_size        = 1;
        message.user_data.raw[0] = i % 255;
        t->eq_uint64_t(0, mq_push(mq, &message), "Pushed message to queue");
    }

    for(size_t i = 0; i < num_messages; ++i) {
        struct Message* msg_peeked = malloc(sizeof(struct Message) + 1);
        msg_peeked->size = sizeof(struct Message) + 1;

        struct Message* msg_popped = malloc(sizeof(struct Message) + 1);
        msg_popped->size = sizeof(struct Message) + 1;

        t->eq_uint64_t(0, mq_peek(mq, msg_peeked), "Peeked message from queue");
        t->eq_uint64_t(0, mq_pop(mq,  msg_popped), "Popped message from queue");

        t->eq_size_t(msg_peeked->size,              msg_popped->size,             "Messages have identical size");
        t->eq_uint8_t(msg_peeked->user_data.raw[0], msg_popped->user_data.raw[0], "Messages have identical raw user data");
        t->eq_uint8_t(msg_peeked->user_data.raw[0], i, "Messages have correct raw user data");

        free(msg_peeked);
        free(msg_popped);
    }

    mq_destroy(mq);
    free(mqs);
}
