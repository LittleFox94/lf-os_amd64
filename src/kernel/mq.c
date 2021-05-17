#include <mq.h>
#include <string.h>
#include <tpa.h>
#include <bluescreen.h>
#include <errno.h>
#include <scheduler.h>

//! Target number of messages per page, muliplied by average message size for allocation size of new pages
static const size_t MessageQueuePageItemsTarget = 16;

static tpa_t*   mqs;
static uint64_t next_mq = 1;

//! This is the header for a message queue page
struct MessageQueuePage {
    //! allocated bytes for the page, excluding header
    size_t allocated;

    //! bytes active in this page (messages not yet popped)
    size_t bytes;

    //! items active in this page (messages not yet popped)
    size_t items;

    //! first byte of first non-popped message
    size_t pop_position;

    //! first free byte in page
    size_t push_position;

    //! Pointer to next page
    struct MessageQueuePage* next;
};

//! This is the implementation data for a message queue
struct MessageQueue {
    //! Maximum number of items in the queue after which mq_push will return false
    size_t max_items;

    //! Maximum number of bytes in non-popped messsages in the queue after which mq_push will return false
    size_t max_bytes;

    //! Counting active-message bytes
    size_t bytes;

    //! Counting active messages
    size_t items;

    //! Average message size, for new page allocation
    size_t average_message_size;

    //! Allocator for new pages
    allocator_t* alloc;

    //! Pointer to first page for popping and peeking
    struct MessageQueuePage* first_page;

    //! Pointer to last page for pushing
    struct MessageQueuePage* last_page;
};

void init_mq(allocator_t* alloc) {
    mqs = tpa_new(alloc, sizeof(struct MessageQueue), 4080, 0);
}

uint64_t mq_create(allocator_t* alloc) {
    if(!next_mq) {
        panic_message("Mutex namespace overflow!");
    }

    struct MessageQueue mq = {
        .alloc   = alloc,

        .max_items            = 0,
        .max_bytes            = 0,
        .bytes                = 0,
        .items                = 0,
        .average_message_size = 0,
        .first_page           = 0,
        .last_page            = 0
    };

    tpa_set(mqs, next_mq, &mq);

    return next_mq++;
}

void mq_destroy(uint64_t mq) {
    struct MessageQueue* data = tpa_get(mqs, mq);

    if(!data) {
        return;
    }

    struct MessageQueuePage* current = data->first_page;
    while(current) {
        struct MessageQueuePage* next = current->next;
        data->alloc->dealloc(data->alloc, current);
        current = next;
    }

    tpa_set(mqs, mq, 0);
}

static void mq_alloc_page(struct MessageQueue* mq, size_t min_size) {
    size_t alloc_size = mq->average_message_size * MessageQueuePageItemsTarget;

    if(alloc_size < min_size) {
        alloc_size = min_size; // let's hope this size is uncommon and next page has average messages again
    }

    struct MessageQueuePage* page = mq->alloc->alloc(mq->alloc, alloc_size + sizeof(struct MessageQueuePage));
    memset(page, 0, alloc_size + sizeof(struct MessageQueuePage));

    page->allocated = alloc_size;

    if(!mq->first_page) {
        mq->first_page = page;
    }

    if(mq->last_page) {
        mq->last_page->next = page;
    }

    mq->last_page = page;
}

uint64_t mq_push(uint64_t mq, struct Message* message) {
    struct MessageQueue* data = tpa_get(mqs, mq);

    if(!data) {
        return ENOENT;
    }

    if((
         data->max_bytes &&
         data->bytes + message->size >= data->max_bytes
       ) ||
       (
         data->max_items &&
         data->items >= data->max_items
       )
    ) {
        return ENOMEM;
    }

    data->average_message_size += message->size;
    if(data->average_message_size != message->size) {
        data->average_message_size /= 2;
    }

    if(!data->last_page ||
        data->last_page->allocated - data->last_page->push_position < message->size) {
        mq_alloc_page(data, message->size);
    }

    void* message_pos = (void*)data->last_page + data->last_page->push_position + sizeof(struct MessageQueuePage);
    memcpy(message_pos, message, message->size); // TODO: either validate size or make sure only the process crashes

    data->last_page->push_position += message->size;
    data->last_page->bytes         += message->size;
    ++data->last_page->items;

    data->bytes += message->size;
    ++data->items;

    union wait_data wd;
    wd.message_queue = mq;
    scheduler_waitable_done(wait_reason_message, wd, 1);

    return 0;
}

uint64_t mq_pop(uint64_t mq, struct Message* msg) {
    uint64_t error;
    if((error = mq_peek(mq, msg))) {
        return error;
    }

    struct MessageQueue* data = tpa_get(mqs, mq);

    data->first_page->pop_position += msg->size;
    data->first_page->bytes        -= msg->size;
    --data->first_page->items;

    data->bytes -= msg->size;
    data->items--;

    if(!data->first_page->items) {
        if(data->first_page == data->last_page) {
            data->last_page = 0;
        }

        struct MessageQueuePage* next = data->first_page->next;
        data->alloc->dealloc(data->alloc, data->first_page);
        data->first_page = next;
    }

    return 0;
}

uint64_t mq_peek(uint64_t mq, struct Message* msg) {
    struct MessageQueue* data = tpa_get(mqs, mq);

    if(!data) {
        return ENOENT;
    }

    if(!data->items) {
        return ENOMSG;
    }

    struct Message* msg_in_page = (struct Message*)((void*)data->first_page + data->first_page->pop_position + sizeof(struct MessageQueuePage));

    if(msg_in_page->size <= msg->size) {
        memcpy(msg, msg_in_page, msg_in_page->size);
        return 0;
    }
    else {
        msg->size = msg_in_page->size;
        msg->type = MT_Invalid;
        return EMSGSIZE;
    }
}
