#ifndef _LFOS_MQ_ROUTER_H_INCLUDED
#define _LFOS_MQ_ROUTER_H_INCLUDED

#include <sys/message_passing.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handler for a single message, may return EAGAIN to requeue a message for
 * later and should return 0 on success. Other return values are undefined.
 */
typedef int(*lfos_mq_router_handler)(struct Message* msg);

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

/**
 * Read all messages currently queued in the kernel for the given MQ.
 *
 * \param mq_router MQ router to read all message pending messages for.
 * \returns errno error code, 0 on success
 */
int lfos_mq_router_receive_messages(struct lfos_mq_router* mq_router);

/**
 * Set the given handler for the given message type. All messages received by this mq_router
 * of the given type will be given to the handler. If messages of the given type are already
 * buffered, they will be passed to the handler while this function executes.
 *
 * If this mq_router already has a handler configured for the given type, this function
 * immediately returns EEXIST.
 *
 * \param mq_router Router to configure this handler on
 * \param type message type this handler should be called for
 * \param handler Handler to call for each message received of the given type
 * \returns errno error code, 0 on success
 */
int lfos_mq_router_set_handler(struct lfos_mq_router* mq_router, enum MessageType type, lfos_mq_router_handler handler);

#ifdef __cplusplus
}
#endif

#endif
