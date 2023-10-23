#include <lfos/mq-router.h>
#include <lfos/dlinkedlist.h>

#if defined(__LF_OS__)
#  include <sys/syscalls.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

struct lfos_mq_router {
    //! MQ to read messages from to distribute to different parts of the program.
    uint64_t mq;

    struct lfos_dlinkedlist* messages;
    struct lfos_dlinkedlist* handlers;
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
        return EBUSY;
    }

    lfos_dlinkedlist_destroy(mq_router->messages);
    lfos_dlinkedlist_destroy(mq_router->handlers);
    free(mq_router);
    return 0;
}

static int lfos_mq_router_receive_message(struct lfos_mq_router* mq_router) {
    size_t size = sizeof(struct Message);
    struct Message* msg = calloc(1, size);
    msg->size = size;

    uint64_t error = EMSGSIZE;

    do {
        if(size != msg->size) {
            size = msg->size;
            msg = realloc(msg, size);
        }

        sc_do_ipc_mq_poll(mq_router->mq, false, msg, &error);
    } while(error == EMSGSIZE);

    if(!error)  {
        struct lfos_dlinkedlist_iterator* it = lfos_dlinkedlist_back(mq_router->messages);
        lfos_dlinkedlist_insert_after(it, msg);
    }

    return error;
}

int lfos_mq_router_receive_messages(struct lfos_mq_router* mq_router) {
    int error = 0;
    while(!error) {
        error = lfos_mq_router_receive_message(mq_router);
#if defined(DEBUG)
        if(error != EAGAIN) {
            fprintf(stderr, "liblfos/mq-router: unexpected error while receiving message: %d\n", error);
        }
#endif
    }

    if(error == EAGAIN) {
        return 0;
    }

    return error;
}

void lfos_mq_router_handle(struct lfos_mq_router* mq_router, struct lfos_dlinkedlist_iterator* msg_it, struct Message* msg, lfos_mq_router_handler handler) {
    int error = 0;
    int ret = handler(msg);
    if(ret != EAGAIN && msg_it) {
        error = lfos_dlinkedlist_unlink(msg_it);
    }
    else if(ret == EAGAIN && !msg_it) {
        struct lfos_dlinkedlist_iterator* back = lfos_dlinkedlist_back(mq_router->messages);
        error = lfos_dlinkedlist_insert_after(back, msg);
    }

#if defined(DEBUG)
    if(error) {
        fprintf(stderr, "liblfos/mq-router: unexpected error after handling message: %d\n", error);
    }
#endif
}

int lfos_mq_router_set_handler(struct lfos_mq_router* mq_router, enum MessageType type, lfos_mq_router_handler handler) {
    struct lfos_mq_router_handler_entry* new_entry = calloc(1, sizeof(struct lfos_mq_router_handler_entry));
    if(!new_entry) {
        return errno;
    }

    new_entry->type = type;
    new_entry->handler = handler;

    struct lfos_dlinkedlist_iterator* it = lfos_dlinkedlist_front(mq_router->handlers);

    while(it) {
        struct lfos_dlinkedlist_iterator* next = lfos_dlinkedlist_next(it);

        struct lfos_mq_router_handler_entry* entry = lfos_dlinkedlist_data(it);
        if(entry && entry->type == type) {
            return EEXIST;
        }

        if(!next) {
            lfos_dlinkedlist_insert_after(it, new_entry);
            break;
        }
        else if(!entry || entry->type > type) {
            lfos_dlinkedlist_insert_before(it, new_entry);
            break;
        }

        it = next;
    }

    it = lfos_dlinkedlist_front(mq_router->messages);
    while(it) {
        struct Message* msg = lfos_dlinkedlist_data(it);
        if(msg->type == type) {
            lfos_mq_router_handle(mq_router, it, msg, handler);
        }

        it = lfos_dlinkedlist_next(it);
    }

    return 0;
}
