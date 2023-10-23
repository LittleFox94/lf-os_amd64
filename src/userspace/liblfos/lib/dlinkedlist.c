#include <lfos/dlinkedlist.h>

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

struct lfos_dlinkedlist_item {
    struct lfos_dlinkedlist_item* prev;
    struct lfos_dlinkedlist_item* next;
    void* data;
};

struct lfos_dlinkedlist {
    struct lfos_dlinkedlist_item* front;
    struct lfos_dlinkedlist_item* back;

    uint64_t generation;
};

struct lfos_dlinkedlist_iterator {
    struct lfos_dlinkedlist* list;
    struct lfos_dlinkedlist_item* item;

    uint64_t generation;
    uint64_t unlinked_generation;
};

static struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_iterator_for_element(
    struct lfos_dlinkedlist* list,
    struct lfos_dlinkedlist_item* item
) {
    struct lfos_dlinkedlist_iterator* ret = calloc(1, sizeof(struct lfos_dlinkedlist_iterator));

    if(ret) {
        ret->list = list;
        ret->item = item;
        ret->generation = list->generation;
    }

    return ret;
}

struct lfos_dlinkedlist* lfos_dlinkedlist_init() {
    return calloc(1, sizeof(struct lfos_dlinkedlist));
}

int lfos_dlinkedlist_destroy(struct lfos_dlinkedlist* list) {
    struct lfos_dlinkedlist_iterator* it = lfos_dlinkedlist_front(list);
    while(it) {
        lfos_dlinkedlist_unlink(it);
        struct lfos_dlinkedlist_iterator* next = lfos_dlinkedlist_next(it);
        free(it);
        it = next;
    }

    free(list);
    return 0;
}

int lfos_dlinkedlist_insert_before(struct lfos_dlinkedlist_iterator* it, void* data) {
    if(it->list->generation != it->generation) {
        return ESTALE;
    }

    struct lfos_dlinkedlist_item* item = calloc(1, sizeof(struct lfos_dlinkedlist_item));
    if(!item) {
        return errno;
    }

    item->data = data;

    if(it->item) {
        item->prev = it->item->prev;
        if(it->item->prev) {
            it->item->prev->next = item;
        }

        item->next = it->item;
        it->item->prev = item;

        if(it->item == it->list->front) {
            it->list->front = item;
        }
    } else {
        it->list->front = item;
        it->list->back = item;
        it->item = item;
    }

    return 0;
}

int lfos_dlinkedlist_insert_after(struct lfos_dlinkedlist_iterator* it, void* data) {
    if(it->list->generation != it->generation) {
        return ESTALE;
    }

    struct lfos_dlinkedlist_item* item = calloc(1, sizeof(struct lfos_dlinkedlist_item));
    if(!item) {
        return errno;
    }

    item->data = data;

    if(it->item) {
        item->next = it->item->next;
        if(it->item->next) {
            it->item->next->prev = item;
        }

        item->prev = it->item;
        it->item->next = item;

        if(it->item == it->list->back) {
            it->list->back = item;
        }
    } else {
        it->list->front = item;
        it->list->back = item;
        it->item = item;
    }

    return 0;
}

int lfos_dlinkedlist_unlink(struct lfos_dlinkedlist_iterator* it) {
    if(it->list->generation != it->generation) {
        return ESTALE;
    }

    if(!it->item) {
        // this is an empty iterator, only used for empty lists
        return 0;
    }

    struct lfos_dlinkedlist_item* item = it->item;

    ++it->list->generation;

    // make this iterator usable for retrieving the next, and only the next, item after unlinking this
    // only valid while the list was not modified another time.
    // This ultimately allows iterating from front to back, unlinking items in the process.
    it->unlinked_generation = it->list->generation;
    it->item = item->next;

    if(item->prev) {
        item->prev->next = item->next;
    }

    if(item->next) {
        item->next->prev = item->prev;
    }

    if(it->list->front == item) {
        it->list->front = item->prev ? item->prev : item->next;
    }

    if(it->list->back == item) {
        it->list->back = item->next ? item->next : item->prev;
    }

    free(item->data);
    free(item);

    return 0;
}

struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_front(struct lfos_dlinkedlist* list) {
    return lfos_dlinkedlist_iterator_for_element(list, list->front);
}

struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_back(struct lfos_dlinkedlist* list) {
    return lfos_dlinkedlist_iterator_for_element(list, list->back);
}

struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_prev(struct lfos_dlinkedlist_iterator* it) {
    struct lfos_dlinkedlist_iterator *new = malloc(sizeof(struct lfos_dlinkedlist_iterator));
    if(!new) {
        return 0;
    }

    memcpy(new, it, sizeof(struct lfos_dlinkedlist_iterator));

    if(lfos_dlinkedlist_backward(new) == 0) {
        return new;
    }

    free(new);
    return 0;
}

struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_next(struct lfos_dlinkedlist_iterator* it) {
    struct lfos_dlinkedlist_iterator *new = malloc(sizeof(struct lfos_dlinkedlist_iterator));
    if(!new) {
        return 0;
    }

    memcpy(new, it, sizeof(struct lfos_dlinkedlist_iterator));

    if(lfos_dlinkedlist_forward(new) == 0) {
        return new;
    }

    free(new);
    return 0;
}

int lfos_dlinkedlist_forward(struct lfos_dlinkedlist_iterator* it) {
    if(it->generation != it->list->generation) {
        // see comment in lfos_dlinkedlist_unlink, basically we allow fetching
        // the next item from an interator who's item was unlinked
        if(it->list->generation == it->unlinked_generation) {
            it->generation = it->list->generation;
            return 0;
        }

        return ESTALE;
    }

    if(it->item && it->item->next) {
        it->item = it->item->next;
    } else {
        return ENOENT;
    }

    return 0;
}

int lfos_dlinkedlist_backward(struct lfos_dlinkedlist_iterator* it) {
    if(it->generation != it->list->generation) {
        return ESTALE;
    }

    if(it->item && it->item->prev) {
        it->item = it->item->prev;
    } else {
        return ENOENT;
    }

    return 0;
}

void* lfos_dlinkedlist_data(struct lfos_dlinkedlist_iterator* it) {
    if(!it) {
        return 0;
    }

    if(it->list->generation != it->generation) {
        return 0;
    }

    if(!it->item) {
        return 0;
    }

    return it->item->data;
}
