#ifndef _LFOS_MQ_ROUTER_H_INCLUDED
#define _LFOS_MQ_ROUTER_H_INCLUDED

#include <sys/message_passing.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MQ router reads messages from a given message queue and route them based on the type of received messages,
 * allowing different parts of applications to use the same message queue (e.g. the process default MQ 0).
 *
 * Additionaly, using the MQ router help moving the messages to userspace memory, allowing more messages to
 * arrive.
 */
struct lfos_mq_router;

/**
 * Initalize MQ router on the given message queue.
 *
 * \param mq ID of the message queue
 * \returns MQ router instance
 */
struct lfos_mq_router* lfos_mq_router_init(uint64_t mq);

/**
 * Tear down a MQ router instance, stoping any processing of new messages.
 *
 * MQ routers cannot be torn down when messages are buffered, make sure to read all messages first.
 *
 * \param mq_router MQ router instance to tear down
 * \returns errno error code, 0 on success.
 */
int lfos_mq_router_destroy(struct lfos_mq_router* mq_router);

#ifdef __cplusplus
}
#endif

#endif
