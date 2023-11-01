#ifndef _MQ_H_INCLUDED
#define _MQ_H_INCLUDED

#include <allocator.h>
#include <stdint.h>
#include <stdbool.h>
#include <message_passing.h>

typedef uint64_t mq_id_t;

typedef void (*mq_notifier)(mq_id_t mq);

void init_mq(allocator_t* alloc);

mq_id_t mq_create(allocator_t* alloc);
void mq_destroy(mq_id_t mq);

uint64_t mq_push(mq_id_t mq, struct Message* message);
uint64_t mq_pop(mq_id_t mq,  struct Message* message);
uint64_t mq_peek(mq_id_t mq, struct Message* message);

uint64_t mq_notify_teardown(mq_id_t mq, mq_notifier notifier);

#endif
