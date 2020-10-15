#ifndef _MQ_H_INCLUDED
#define _MQ_H_INCLUDED

#include <allocator.h>
#include <stdint.h>
#include <stdbool.h>
#include <message_passing.h>

struct MessageQueue {
    //! Maximum number of items in the queue after which mq_push will return false
    size_t max_items;

    //! Maximum number of bytes in non-popped messsages in the queue after which mq_push will return false
    size_t max_bytes;

    //! don't touch this!
    uint8_t _implData[128]; // XXX: yes this is ugly
};

void mq_create(struct MessageQueue* mq, allocator_t* alloc, deallocator_t* dealloc);

bool mq_push(struct MessageQueue* mq, struct Message* message);
struct Message* mq_pop(struct MessageQueue* mq);
struct Message* mq_peek(struct MessageQueue* mq);

#endif
