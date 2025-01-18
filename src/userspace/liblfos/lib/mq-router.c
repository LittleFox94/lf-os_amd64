#include <lfos/mq-router.h>
#include <lfos/dlinkedlist.h>

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#if defined(__LF_OS__)
#  include <sys/syscalls.h>
#else
    void sc_do_ipc_mq_poll(uint64_t mq, bool block, void* msg, uint64_t* error);
#endif

struct lfos_mq_router {
    //! MQ to read messages from to distribute to different parts of the program.
    uint64_t mq;

    struct lfos_dlinkedlist* messages;
    struct lfos_dlinkedlist* handlers;

    bool has_requeued_messages;
};

struct lfos_mq_router_handler_entry {
    enum MessageType type;
    lfos_mq_router_handler handler;
};

struct lfos_mq_router* lfos_mq_router_init(uint64_t mq) {
    struct lfos_mq_router* ret = calloc(1, sizeof(struct lfos_mq_router));
    ret->mq = mq;
    ret->messages = lfos_dlinkedlist_init();
    ret->handlers = lfos_dlinkedlist_init();

    if(!ret->messages || !ret->handlers) {
        if(ret->messages) {
            lfos_dlinkedlist_destroy(ret->messages);
        }

        if(ret->handlers) {
            lfos_dlinkedlist_destroy(ret->handlers);
        }

        free(ret);
        return 0;
    }

    return ret;
}

int lfos_mq_router_destroy(struct lfos_mq_router* mq_router) {
    struct lfos_dlinkedlist_iterator* front = lfos_dlinkedlist_front(mq_router->messages);
    if(lfos_dlinkedlist_data(front)) {
        free(front);
        return EBUSY;
    }

    free(front);

    lfos_dlinkedlist_destroy(mq_router->messages);
    lfos_dlinkedlist_destroy(mq_router->handlers);
    free(mq_router);
    return 0;
}

static lfos_mq_router_handler lfos_mq_router_handler_by_message_type(struct lfos_mq_router* mq_router, enum MessageType type) {
    struct lfos_dlinkedlist_iterator* it = lfos_dlinkedlist_front(mq_router->handlers);

    int error = 0;
    lfos_mq_router_handler handler = 0;
    while(!error) {
        struct lfos_mq_router_handler_entry* entry = lfos_dlinkedlist_data(it);
        if(entry && entry->type == type) {
            handler = entry->handler;
            break;
        }

        error = lfos_dlinkedlist_forward(it);
    }
    free(it);

    return handler;
}

static void lfos_mq_router_handle(struct lfos_mq_router* mq_router, struct lfos_dlinkedlist_iterator* msg_it, struct Message* msg) {
    lfos_mq_router_handler handler = lfos_mq_router_handler_by_message_type(mq_router, msg->type);

    int error = 0;
    bool queue = !handler;

    if(handler) {
        error = handler(msg);

        if(error == EAGAIN) {
            mq_router->has_requeued_messages = true;

            if(!msg_it) {
                queue = true;
            }
        } else if(msg_it) {
            error = lfos_dlinkedlist_unlink(msg_it);
        } else {
            free(msg);
        }
    }

    if(queue) {
        struct lfos_dlinkedlist_iterator* back = lfos_dlinkedlist_back(mq_router->messages);
        error = lfos_dlinkedlist_insert_after(back, msg);
        free(back);
    }

#if defined(DEBUG)
    if(error) {
        fprintf(stderr, "liblfos/mq-router: unexpected error after handling message: %d\n", error);
    }
#endif
}

static int lfos_mq_router_receive_message(struct lfos_mq_router* mq_router) {
    size_t size = sizeof(struct Message);
    struct Message* msg = calloc(1, size);
    if(!msg) {
        return errno;
    }

    msg->size = size;

    uint64_t error = EMSGSIZE;

    do {
        if(size != msg->size) {
            size = msg->size;
            struct Message* new_msg = realloc(msg, size);
            if(!new_msg) {
                error = errno;
                free(msg);
                return error;
            }

            msg = new_msg;
        }

        sc_do_ipc_mq_poll(mq_router->mq, false, msg, &error);
    } while(error == EMSGSIZE);

    if(!error)  {
        lfos_mq_router_handle(mq_router, 0, msg);
    } else {
        free(msg);
    }

    return error;
}

static void lfos_mq_router_process_requeued_messages(struct lfos_mq_router* mq_router) {
    if(!mq_router->has_requeued_messages) {
        return;
    }

    mq_router->has_requeued_messages = false;

    struct lfos_dlinkedlist_iterator* it = lfos_dlinkedlist_front(mq_router->messages);
    int error = 0;
    while(!error) {
        struct Message* msg = lfos_dlinkedlist_data(it);
        if(msg) {
            lfos_mq_router_handle(mq_router, it, msg);
        } else {
            break;
        }

        error = lfos_dlinkedlist_forward(it);
    }
    free(it);
}

int lfos_mq_router_receive_messages(struct lfos_mq_router* mq_router) {
    lfos_mq_router_process_requeued_messages(mq_router);

    int error = 0;
    do {
        error = lfos_mq_router_receive_message(mq_router);
#if defined(DEBUG)
        if(error != EAGAIN) {
            fprintf(stderr, "liblfos/mq-router: unexpected error while receiving message: %d\n", error);
        }
#endif
    } while(!error && error != EAGAIN);

    if(error == EAGAIN) {
        return 0;
    }

    return error;
}

int lfos_mq_router_set_handler(struct lfos_mq_router* mq_router, enum MessageType type, lfos_mq_router_handler handler) {
    struct lfos_mq_router_handler_entry* new_entry = calloc(1, sizeof(struct lfos_mq_router_handler_entry));
    if(!new_entry) {
        return errno;
    }

    new_entry->type = type;
    new_entry->handler = handler;

    struct lfos_dlinkedlist_iterator* it = lfos_dlinkedlist_front(mq_router->handlers);

    int error = 0;
    while(!error) {
        struct lfos_mq_router_handler_entry* entry = lfos_dlinkedlist_data(it);
        if(entry && entry->type == type) {
            return EEXIST;
        }

        if(!entry || entry->type > type) {
            lfos_dlinkedlist_insert_before(it, new_entry);
            break;
        }

        error = lfos_dlinkedlist_forward(it);
        struct lfos_mq_router_handler_entry* nextEntry = lfos_dlinkedlist_data(it);

        if(error) {
            lfos_dlinkedlist_insert_after(it, new_entry);
            break;
        }
    }

    free(it);

    error = 0;
    it = lfos_dlinkedlist_front(mq_router->messages);
    while(!error) {
        struct Message* msg = lfos_dlinkedlist_data(it);
        if(!msg) {
            // nothing queued
            break;
        }

        if(msg->type == type) {
            lfos_mq_router_handle(mq_router, it, msg);
        }

        error = lfos_dlinkedlist_forward(it);
    }
    free(it);

    return 0;
}
