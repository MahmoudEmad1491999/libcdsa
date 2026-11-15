#ifndef CDSA_DLIST_H
#define CDSA_DLIST_H

#include <stddef.h>

#include "cdsa/cdsa_callbacks.h"
#include "cdsa/cdsa_status.h"

/**
 * @file cdsa_dlist.h
 * @brief Doubly linked list with a sentinel head node.
 *
 * cdsa_dlist_t is a doubly linked list with a dummy sentinel head node so that
 * every structural operation (push, pop, insert, remove) requires no
 * special-casing for empty lists or boundary positions.  A tail pointer gives
 * O(1) push_back and pop_back.
 *
 * The doubly linked structure enables O(1) insert_before, insert_after, and
 * remove_node (all given a node pointer), and O(1) splice of a whole list.
 *
 * ### Ownership model
 * On push or insert, elem_size bytes are copied from the caller's buffer into a
 * fresh heap node.  Supplying a @p free_fn at creation time allows the list to
 * release deep resources when elements are discarded.
 *
 * ### Thread-safety
 * No internal locking.  Concurrent access requires external synchronisation.
 */
typedef struct cdsa_dlist      cdsa_dlist_t;
typedef struct cdsa_dlist_node cdsa_dlist_node_t;

/* ── Create / destroy ────────────────────────────────────────────────────── */

/**
 * @brief Create an empty doubly linked list.
 *
 * @param elem_size  Size of each element in bytes.  Must be > 0.
 * @param cmp_fn     Comparator used by cdsa_dlist_find() and
 *                   cdsa_dlist_remove().  May be NULL if those operations
 *                   are never called.
 * @param free_fn    Destructor called when an element is discarded without an
 *                   output buffer, or when the list is destroyed.  May be NULL.
 * @param out        Receives the new list pointer.  Must not be NULL.
 * @return           CDSA_OK on success; CDSA_ERR_INVAL if elem_size == 0 or
 *                   out is NULL; CDSA_ERR_NOMEM if allocation fails.
 * @complexity       O(1) time, O(1) space.
 */
cdsa_status_t cdsa_dlist_create(size_t elem_size, cdsa_cmp_fn cmp_fn,
                                 cdsa_free_fn free_fn, cdsa_dlist_t **out);

/**
 * @brief Destroy the list and all its nodes.
 *
 * If free_fn was provided, it is called on every remaining element before the
 * node is freed.  Passing NULL is a no-op.
 *
 * @param list  List to destroy, or NULL.
 * @complexity  O(n).
 */
void cdsa_dlist_destroy(cdsa_dlist_t *list);

/* ── Push / pop front ────────────────────────────────────────────────────── */

/**
 * @brief Prepend a copy of @p elem at the front of the list.
 *
 * @param list  Target list.  Must not be NULL.
 * @param elem  Source buffer of at least elem_size bytes.  Must not be NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if list or elem is NULL;
 *              CDSA_ERR_NOMEM if the node allocation fails.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_dlist_push_front(cdsa_dlist_t *list, const void *elem);

/**
 * @brief Remove and optionally return the front element.
 *
 * If @p out is non-NULL, elem_size bytes are copied into it and free_fn is
 * NOT called.  If @p out is NULL and free_fn is set, free_fn is called.
 *
 * @param list  Source list.  Must not be NULL.
 * @param out   Destination buffer (at least elem_size bytes), or NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if list is NULL;
 *              CDSA_ERR_EMPTY if the list is empty.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_dlist_pop_front(cdsa_dlist_t *list, void *out);

/* ── Push / pop back ─────────────────────────────────────────────────────── */

/**
 * @brief Append a copy of @p elem at the back of the list.
 *
 * @param list  Target list.  Must not be NULL.
 * @param elem  Source buffer of at least elem_size bytes.  Must not be NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if list or elem is NULL;
 *              CDSA_ERR_NOMEM if the node allocation fails.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_dlist_push_back(cdsa_dlist_t *list, const void *elem);

/**
 * @brief Remove and optionally return the back element.
 *
 * Same out / free_fn semantics as cdsa_dlist_pop_front().
 *
 * @param list  Source list.  Must not be NULL.
 * @param out   Destination buffer (at least elem_size bytes), or NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if list is NULL;
 *              CDSA_ERR_EMPTY if the list is empty.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_dlist_pop_back(cdsa_dlist_t *list, void *out);

/* ── Peek (non-destructive) ──────────────────────────────────────────────── */

/**
 * @brief Copy the front element into @p out without removing it.
 *
 * @param list  Source list.  Must not be NULL.
 * @param out   Destination buffer (at least elem_size bytes).  Must not be NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if list or out is NULL;
 *              CDSA_ERR_EMPTY if the list is empty.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_dlist_peek_front(const cdsa_dlist_t *list, void *out);

/**
 * @brief Copy the back element into @p out without removing it.
 *
 * @param list  Source list.  Must not be NULL.
 * @param out   Destination buffer (at least elem_size bytes).  Must not be NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if list or out is NULL;
 *              CDSA_ERR_EMPTY if the list is empty.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_dlist_peek_back(const cdsa_dlist_t *list, void *out);

/* ── Node-based insert ───────────────────────────────────────────────────── */

/**
 * @brief Insert a copy of @p elem immediately before @p node.
 *
 * @p node must be a valid non-NULL node belonging to @p list.  Passing the
 * list's head node inserts at the front.
 *
 * @param list  Target list.  Must not be NULL.
 * @param node  Existing node to insert before.  Must not be NULL.
 * @param elem  Source buffer of at least elem_size bytes.  Must not be NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if any argument is NULL;
 *              CDSA_ERR_NOMEM if the node allocation fails.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_dlist_insert_before(cdsa_dlist_t *list,
                                        cdsa_dlist_node_t *node,
                                        const void *elem);

/**
 * @brief Insert a copy of @p elem immediately after @p node.
 *
 * @p node must be a valid non-NULL node belonging to @p list.  Passing the
 * list's tail node appends at the back.
 *
 * @param list  Target list.  Must not be NULL.
 * @param node  Existing node to insert after.  Must not be NULL.
 * @param elem  Source buffer of at least elem_size bytes.  Must not be NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if any argument is NULL;
 *              CDSA_ERR_NOMEM if the node allocation fails.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_dlist_insert_after(cdsa_dlist_t *list,
                                       cdsa_dlist_node_t *node,
                                       const void *elem);

/* ── Node-based remove ───────────────────────────────────────────────────── */

/**
 * @brief Remove a specific node from the list.
 *
 * Same out / free_fn semantics as cdsa_dlist_pop_front().  Unlike the
 * key-based cdsa_dlist_remove(), this operation is O(1) because the node
 * pointer is provided directly.
 *
 * @param list  Source list.  Must not be NULL.
 * @param node  Node to remove.  Must not be NULL; must belong to @p list.
 * @param out   Destination buffer (at least elem_size bytes), or NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if list or node is NULL.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_dlist_remove_node(cdsa_dlist_t *list,
                                      cdsa_dlist_node_t *node,
                                      void *out);

/* ── Find / remove by key ────────────────────────────────────────────────── */

/**
 * @brief Return a pointer to the first node whose element compares equal to
 *        @p key (using the list's cmp_fn).
 *
 * @param list      Source list.  Must not be NULL.
 * @param key       Key to search for.  Must not be NULL.
 * @param node_out  Receives a pointer to the matching node.  Must not be NULL.
 * @return          CDSA_OK on success; CDSA_ERR_INVAL if any arg is NULL or
 *                  cmp_fn was not set at creation; CDSA_ERR_NOTFOUND if no
 *                  element matches.
 * @complexity      O(n) time.
 */
cdsa_status_t cdsa_dlist_find(const cdsa_dlist_t *list, const void *key,
                               cdsa_dlist_node_t **node_out);

/**
 * @brief Remove the first element that compares equal to @p key.
 *
 * Same out / free_fn semantics as cdsa_dlist_pop_front().
 *
 * @param list  Target list.  Must not be NULL.
 * @param key   Key to remove.  Must not be NULL.
 * @param out   Destination buffer (at least elem_size bytes), or NULL.
 * @return      CDSA_OK on success; CDSA_ERR_INVAL if list or key is NULL or
 *              cmp_fn was not set at creation; CDSA_ERR_NOTFOUND if no
 *              element matches.
 * @complexity  O(n) time.
 */
cdsa_status_t cdsa_dlist_remove(cdsa_dlist_t *list, const void *key, void *out);

/* ── Splice ──────────────────────────────────────────────────────────────── */

/**
 * @brief Move all nodes of @p src into @p dest immediately before @p pos.
 *
 * Both lists must have the same elem_size.  After a successful call, @p src is
 * empty and @p dest contains all elements that were in @p src.  Passing
 * @p pos == NULL appends all elements to the back of @p dest.
 *
 * The operation does NOT copy element data; it relinks existing nodes.
 *
 * @param dest  Destination list.  Must not be NULL and must differ from @p src.
 * @param pos   Node in @p dest to insert before, or NULL to append.
 * @param src   Source list.  Must not be NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if dest or src is NULL, if dest == src,
 *              or if elem_sizes differ.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_dlist_splice(cdsa_dlist_t *dest, cdsa_dlist_node_t *pos,
                                 cdsa_dlist_t *src);

/* ── Reverse ─────────────────────────────────────────────────────────────── */

/**
 * @brief Reverse the list in place.
 *
 * @param list  List to reverse.  Must not be NULL.
 * @complexity  O(n) time, O(1) extra space.
 */
void cdsa_dlist_reverse(cdsa_dlist_t *list);

/* ── Accessors ───────────────────────────────────────────────────────────── */

/**
 * @brief Return the number of elements in the list.
 *
 * @param list  Source list (NULL returns 0).
 * @return      Element count.
 * @complexity  O(1) time.
 */
size_t cdsa_dlist_len(const cdsa_dlist_t *list);

/**
 * @brief Return a pointer to the first real node, or NULL if the list is empty.
 *
 * @param list  Source list (NULL returns NULL).
 * @return      First node, or NULL.
 * @complexity  O(1) time.
 */
cdsa_dlist_node_t *cdsa_dlist_head(const cdsa_dlist_t *list);

/**
 * @brief Return a pointer to the last real node, or NULL if the list is empty.
 *
 * @param list  Source list (NULL returns NULL).
 * @return      Last node, or NULL.
 * @complexity  O(1) time.
 */
cdsa_dlist_node_t *cdsa_dlist_tail(const cdsa_dlist_t *list);

/**
 * @brief Return the successor of @p node, or NULL if @p node is the last.
 *
 * @param node  Current node (NULL returns NULL).
 * @return      Next node, or NULL.
 * @complexity  O(1) time.
 */
cdsa_dlist_node_t *cdsa_dlist_node_next(cdsa_dlist_node_t *node);

/**
 * @brief Return the predecessor of @p node, or NULL if @p node is the first.
 *
 * @param node  Current node (NULL returns NULL).
 * @return      Previous node, or NULL.
 * @complexity  O(1) time.
 */
cdsa_dlist_node_t *cdsa_dlist_node_prev(cdsa_dlist_node_t *node);

/**
 * @brief Return a pointer to the element data stored in @p node.
 *
 * @param node  Node whose data to access.  Must not be NULL.
 * @return      Pointer to elem_size bytes of element data.
 * @complexity  O(1) time.
 */
void *cdsa_dlist_node_data(cdsa_dlist_node_t *node);

/* ── Iteration macros ────────────────────────────────────────────────────── */

/**
 * @brief Iterate over every node of the list front-to-back.
 *
 * @p node_ is set to each @c cdsa_dlist_node_t* in turn.  Declare it in the
 * enclosing scope before using this macro.  Modifying the list (push, pop,
 * remove) during iteration produces undefined behaviour.  @c break and
 * @c continue work as expected.
 *
 * @param list_  @c cdsa_dlist_t* to iterate over.
 * @param node_  @c cdsa_dlist_node_t* lvalue set to each node.
 * @complexity   O(1) per node; O(n) for a complete traversal.
 */
#define CDSA_DLIST_FOREACH(list_, node_)                              \
    for ((node_) = cdsa_dlist_head(list_);                            \
         (node_) != NULL;                                             \
         (node_) = cdsa_dlist_node_next(node_))

/**
 * @brief Iterate over every node of the list back-to-front.
 *
 * @p node_ is set to each @c cdsa_dlist_node_t* in reverse order.  Same
 * usage and caveats as @c CDSA_DLIST_FOREACH.
 *
 * @param list_  @c cdsa_dlist_t* to iterate over.
 * @param node_  @c cdsa_dlist_node_t* lvalue set to each node.
 * @complexity   O(1) per node; O(n) for a complete traversal.
 */
#define CDSA_DLIST_FOREACH_REVERSE(list_, node_)                      \
    for ((node_) = cdsa_dlist_tail(list_);                            \
         (node_) != NULL;                                             \
         (node_) = cdsa_dlist_node_prev(node_))

#endif /* CDSA_DLIST_H */
