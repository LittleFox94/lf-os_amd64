#include <lfostest.h>
#include <mq.c>

__attribute__ ((visibility ("default"))) void testmain(TestUtils* t) {
    struct MessageQueue mq;
    t->lt_size_t(sizeof(mq._implData), sizeof(struct MessageQueueImpl), "MessageQueueImpl fits in the space for it");

    mq_create(&mq, malloc, free);

    struct Message message;
    message.size             = 1 + sizeof(message);
    message.user_size        = 1;
    message.type             = MT_UserDefined;
    message.user_data.raw[0] = 0xa;

    mq.max_items = 0;
    mq.max_bytes = message.size;
    t->eq_bool(false, mq_push(&mq, &message), "Correctly denied adding to item-full queue");
    t->eq_ptr_t(0,  (ptr_t)mq_peek(&mq),      "No data in queue, by peek");

    mq.max_items = 1;
    mq.max_bytes = 0;
    t->eq_bool(false, mq_push(&mq, &message), "Correctly denied adding to byte-full queue");
    t->eq_ptr_t(0,   (ptr_t)mq_peek(&mq),     "No data in queue, by peek");

    mq.max_items = 0;
    mq.max_bytes = 0;
    t->eq_bool(true, mq_push(&mq, &message), "Added message to unlimited queue");
    struct Message* msg = mq_peek(&mq);
    t->ne_ptr_t(0,   (ptr_t)msg,    "Message in queue, by peek");
    free(msg);

    const size_t num_messages = 16;
    for(size_t i = 0; i < num_messages; ++i) {
        message.user_data.raw[0] = i % 255;
        mq_push(&mq, &message);
    }

    for(size_t i = 0; i < num_messages; ++i) {
        struct Message* msg_peeked = mq_peek(&mq);
        struct Message* msg_popped = mq_pop(&mq);

        t->ne_ptr_t((ptr_t)msg_peeked, (ptr_t)msg_popped, "Peek & pop return different pointers -> messages copied");

        t->eq_size_t(msg_peeked->size,              msg_popped->size,             "Messages have identical size");
        t->eq_uint8_t(msg_peeked->user_data.raw[0], msg_popped->user_data.raw[0], "Messages have identical raw user data");

        free(msg_peeked);
        free(msg_popped);
    }

    msg = mq_pop(&mq);
    t->ne_ptr_t(0, (ptr_t)msg, "Message in queue, popped it");
    free(msg);
}
