#ifndef _LFOS_DLINKEDLIST_H_INCLUDED
#define _LFOS_DLINKEDLIST_H_INCLUDED

struct lfos_dlinkedlist;
struct lfos_dlinkedlist_iterator;

struct lfos_dlinkedlist* lfos_dlinkedlist_init();

int lfos_dlinkedlist_destroy(struct lfos_dlinkedlist* list);

struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_front(struct lfos_dlinkedlist* list);
struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_back(struct lfos_dlinkedlist* list);

int lfos_dlinkedlist_insert_before(struct lfos_dlinkedlist_iterator* it, void* data);
int lfos_dlinkedlist_insert_after(struct lfos_dlinkedlist_iterator* it, void* data);

struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_next(struct lfos_dlinkedlist_iterator* it);
struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_prev(struct lfos_dlinkedlist_iterator* it);
void* lfos_dlinkedlist_data(struct lfos_dlinkedlist_iterator* it);
int lfos_dlinkedlist_unlink(struct lfos_dlinkedlist_iterator* it);


#endif // _LFOS_DLINKEDLIST_H_INCLUDED
