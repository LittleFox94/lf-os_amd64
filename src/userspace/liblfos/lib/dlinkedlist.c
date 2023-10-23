#include <lfos/dlinkedlist.h>

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
};

struct lfos_dlinkedlist_iterator {
    struct lfos_dlinkedlist* list;
    struct lfos_dlinkedlist_item* item;
};

static struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_iterator_for_element(
    struct lfos_dlinkedlist* list,
    struct lfos_dlinkedlist_item* item
) {
    struct lfos_dlinkedlist_iterator* ret = calloc(1, sizeof(struct lfos_dlinkedlist_iterator));

    if(ret) {
        ret->list = list;
        ret->item = item;
    }

    return ret;
}

struct lfos_dlinkedlist* lfos_dlinkedlist_init() {
    return calloc(0, sizeof(struct lfos_dlinkedlist));
}

int lfos_dlinkedlist_destroy(struct lfos_dlinkedlist* list) {
    struct lfos_dlinkedlist_iterator* it = lfos_dlinkedlist_front(list);
    while(it) {
        struct lfos_dlinkedlist_iterator* next = lfos_dlinkedlist_next(it);
        lfos_dlinkedlist_unlink(it);
        free(it);
        it = next;
    }

    return 0;
}

int lfos_dlinkedlist_insert_before(struct lfos_dlinkedlist_iterator* it, void* data) {
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
    }

    return 0;
}

int lfos_dlinkedlist_insert_after(struct lfos_dlinkedlist_iterator* it, void* data) {
    struct lfos_dlinkedlist_item* item = calloc(1, sizeof(struct lfos_dlinkedlist_item));
    if(!item) {
        return errno;
    }

    item->data = data;

    if(it->item) {
        item->prev = it->item;
        if(it->item->next) {
            it->item->next->next = item;
        }

        item->prev = it->item;
        it->item->next = item;

        if(it->item == it->list->back) {
            it->list->back = item;
        }
    } else {
        it->list->front = item;
        it->list->back = item;
    }

    return 0;
}

int lfos_dlinkedlist_unlink(struct lfos_dlinkedlist_iterator* it) {
    if(it->item->prev) {
        it->item->prev->next = it->item->next;
    }

    if(it->item->next) {
        it->item->next->prev = it->item->prev;
    }

    free(it->item->data);
    free(it->item);
    return 0;
}

struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_front(struct lfos_dlinkedlist* list) {
    return lfos_dlinkedlist_iterator_for_element(list, list->front);
}

struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_back(struct lfos_dlinkedlist* list) {
    return lfos_dlinkedlist_iterator_for_element(list, list->back);
}

struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_prev(struct lfos_dlinkedlist_iterator* it) {
    if(it->item->prev) {
        return lfos_dlinkedlist_iterator_for_element(it->list, it->item->prev);
    }
    return 0;
}

struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_next(struct lfos_dlinkedlist_iterator* it) {
    if(it->item->next) {
        return lfos_dlinkedlist_iterator_for_element(it->list, it->item->next);
    }
    return 0;
}

void* lfos_dlinkedlist_data(struct lfos_dlinkedlist_iterator* it) {
    return it->item->data;
}
