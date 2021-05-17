#ifndef _MQ_H_INCLUDED
#define _MQ_H_INCLUDED

#include <allocator.h>
#include <stdint.h>
#include <stdbool.h>
#include <message_passing.h>

void init_mq(allocator_t* alloc);

uint64_t mq_create(allocator_t* alloc);
void mq_destroy(uint64_t mq);

uint64_t mq_push(uint64_t mq, struct Message* message);
uint64_t mq_pop(uint64_t mq,  struct Message* message);
uint64_t mq_peek(uint64_t mq, struct Message* message);

#endif
