#ifndef _LFOS_DLINKEDLIST_H_INCLUDED
#define _LFOS_DLINKEDLIST_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//! A double-linked list, to be used with the lfos_dlinkedlist_* functions
struct lfos_dlinkedlist;

/**
 * An iterator of a a double-linked list, to be used with the lfos_dlinkedlist_* functions.
 *
 * \remarks remember to free() any iterator no longer in use!
 */
struct lfos_dlinkedlist_iterator;

/**
 * Initializes a new double-linked list.
 *
 * \returns handle to the double-linked list to use with the other lfos_dlinkedlist_* functions.
 */
struct lfos_dlinkedlist* lfos_dlinkedlist_init();

/**
 * Destroys a given double-linked list, freeing all memory used by it.
 *
 * \param list double-linked list to destroy
 * \returns error code (errno)
 */
int lfos_dlinkedlist_destroy(struct lfos_dlinkedlist* list);

/**
 * Retrieve an iterator for the front (first item) of the list.
 *
 * \param list double-linked list to retrieve an iterator for.
 * \returns iterator for the first item of the given list
 */
struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_front(struct lfos_dlinkedlist* list);

/**
 * Retrieve an iterator for the back (last item) of the list.
 *
 * \param list double-linked list to retrieve an iterator for.
 * \returns iterator for the last item of the given list
 */
struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_back(struct lfos_dlinkedlist* list);

/**
 * Insert an item into the list before the item the given iterator points at.
 *
 * \param it iterator to use as reference where to place the new element
 * \param data data to store in the list
 * \return error code (errno)
 */
int lfos_dlinkedlist_insert_before(struct lfos_dlinkedlist_iterator* it, void* data);

/**
 * Insert an item into the list after the item the given iterator points at.
 *
 * \param it iterator to use as reference where to place the new element
 * \param data data to store in the list
 * \return error code (errno)
 */
int lfos_dlinkedlist_insert_after(struct lfos_dlinkedlist_iterator* it, void* data);

/**
 * Remove the item pointed at by the given iterator from the list.
 *
 * \remarks Invalidates all iterators but as a special case, you can call
 * lfos_dlinkedlist_forward or lfos_dlinkedlist_next on the given iterator to
 * retrieve the element after the one just deleted from the list, allowing you
 * to iterate from front to back and deleting items in the process.
 *
 * \param it iterator pointing at the item to delete from the list
 * \return error code (errno)
 */
int lfos_dlinkedlist_unlink(struct lfos_dlinkedlist_iterator* it);

/**
 * Retrieve an iterator for the element following the one the given iterator is pointing at.
 *
 * \remarks Forwarding the iterator with lfos_dlinkedlist_forward is much more efficient.
 *
 * \param it iterator of which a new iterator for the following item will be created.
 * \return iterator for the item following the one pointed at by the given iterator, NULL if no item following or any other error occurs.
 */
struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_next(struct lfos_dlinkedlist_iterator* it);

/**
 * Retrieve an iterator for the element before the one the given iterator is pointing at.
 *
 * \remarks Backwarding the iterator with lfos_dlinkedlist_backward is much more efficient.
 *
 * \param it iterator of which a new iterator for the previous item will be created.
 * \return iterator for the item before the one pointed at by the given iterator, NULL if no previous item or any other error occurs.
 */
struct lfos_dlinkedlist_iterator* lfos_dlinkedlist_prev(struct lfos_dlinkedlist_iterator* it);

/**
 * Modify given iterator to point at the item following the one it is currently pointing at.
 *
 * \return error code (errno), e.g. ENOENT if no item is following
 */
int lfos_dlinkedlist_forward(struct lfos_dlinkedlist_iterator* it);

/**
 * Modify given iterator to point at the item before the one it is currently pointing at.
 *
 * \return error code (errno), e.g. ENOENT if no previous item
 */
int lfos_dlinkedlist_backward(struct lfos_dlinkedlist_iterator* it);

/**
 * Retrieve the data of the item the given iterator is pointing at.
 *
 * \param it iterator pointing at the item for which the data should be retrieved.
 * \return data of the item pointed at by the given iterator, NULL on any error.
 */
void* lfos_dlinkedlist_data(struct lfos_dlinkedlist_iterator* it);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _LFOS_DLINKEDLIST_H_INCLUDED
