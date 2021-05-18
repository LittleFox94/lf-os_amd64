#ifndef _SD_H_INCLUDED
#define _SD_H_INCLUDED

#include <uuid.h>
#include <mq.h>

void init_sd();

uint64_t sd_register(uuid_t* uuid, mq_id_t mq);
int64_t sd_send(uuid_t* uuid, struct Message* msg);

#endif
