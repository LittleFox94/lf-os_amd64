#include <mq.h>
#include <string.h>

//! Target number of messages per page, muliplied by average message size for allocation size of new pages
static const size_t MQ_PAGE_ITEMS_TARGET = 16;

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
struct MessageQueueImpl {
    //! Counting active-message bytes
    size_t bytes;

    //! Counting active messages
    size_t items;

    //! Average message size, for new page allocation
    size_t average_message_size;

    //! Allocator for new pages
    allocator_t* alloc;

    //! How to free pages
    deallocator_t* dealloc;

    //! Pointer to first page for popping and peeking
    struct MessageQueuePage* first_page;

    //! Pointer to last page for pushing
    struct MessageQueuePage* last_page;
};

#define IMPL ((struct MessageQueueImpl*)(&mq->_implData))

void mq_create(struct MessageQueue* mq, allocator_t* alloc, deallocator_t* dealloc) {
    memset(mq, 0, sizeof(struct MessageQueue));

    IMPL->alloc   = alloc;
    IMPL->dealloc = dealloc;
}

static void mq_alloc_page(struct MessageQueue* mq, size_t min_size) {
    size_t alloc_size = IMPL->average_message_size * MQ_PAGE_ITEMS_TARGET;

    if(alloc_size < min_size) {
        alloc_size = min_size; // let's hope this size is uncommon and next page has average messages again
    }

    struct MessageQueuePage* page = IMPL->alloc(alloc_size + sizeof(struct MessageQueuePage));
    memset(page, 0, alloc_size + sizeof(struct MessageQueuePage));

    page->allocated = alloc_size;

    if(!IMPL->first_page) {
        IMPL->first_page = page;
    }

    if(IMPL->last_page) {
        IMPL->last_page->next = page;
    }

    IMPL->last_page = page;
}

bool mq_push(struct MessageQueue* mq, struct Message* message) {
    if((
         mq->max_bytes ||
         mq->max_items
       ) &&
       (
         IMPL->bytes + message->size >= mq->max_bytes ||
         IMPL->items                 >= mq->max_items
       )
    ) {
        return false;
    }

    IMPL->average_message_size += message->size;
    if(IMPL->average_message_size != message->size) {
        IMPL->average_message_size /= 2;
    }

    if(!IMPL->last_page ||
        IMPL->last_page->allocated - IMPL->last_page->push_position <= message->size) {
        mq_alloc_page(mq, message->size);
    }

    void* message_pos = (void*)IMPL->last_page + IMPL->last_page->push_position + sizeof(struct MessageQueueImpl);
    memcpy(message_pos, message, message->size); // XXX: either validate size or make sure only the process crashes

    IMPL->last_page->push_position += message->size;
    IMPL->last_page->bytes         += message->size;
    ++IMPL->last_page->items;

    IMPL->bytes += message->size;
    ++IMPL->items;

    return true;
}

struct Message* mq_pop(struct MessageQueue* mq) {
    if(!IMPL->items) {
        return 0;
    }

    struct Message* ret = mq_peek(mq);

    IMPL->first_page->pop_position += ret->size;
    IMPL->first_page->bytes        -= ret->size;
    --IMPL->first_page->items;

    IMPL->bytes -= ret->size;
    IMPL->items -= ret->size;

    if(!IMPL->first_page->items) {
        if(IMPL->first_page == IMPL->last_page) {
            IMPL->last_page = 0;
        }

        struct MessageQueuePage* next = IMPL->first_page->next;
        IMPL->dealloc(IMPL->first_page);
        IMPL->first_page = next;
    }

    return ret;
}

struct Message* mq_peek(struct MessageQueue* mq) {
    if(!IMPL->items) {
        return 0;
    }

    struct Message* msg_in_page = (struct Message*)((void*)IMPL->first_page + IMPL->first_page->pop_position + sizeof(struct MessageQueueImpl));
    struct Message* msg_allocated = IMPL->alloc(msg_in_page->size);
    memcpy(msg_allocated, msg_in_page, msg_in_page->size);

    return msg_allocated;
}
