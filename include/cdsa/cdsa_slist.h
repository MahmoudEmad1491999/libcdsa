#ifndef CDSA_SLIST_H
#define CDSA_SLIST_H

#include <stddef.h>

#include "cdsa/cdsa_callbacks.h"
#include "cdsa/cdsa_status.h"

/**
 * @file cdsa_slist.h
 * @brief Singly linked list with a sentinel head node.
 *
 * cdsa_slist_t is an intrusive singly linked list that uses a dummy "sentinel"
 * head node so that push/pop/remove never need to special-case an empty list.
 * A @p tail pointer is maintained for O(1) push_back; pop_back is O(n).
 *
 * ### Ownership model
 * On push, elem_size bytes are copied from the caller's buffer into a fresh
 * heap node.  Supplying a @p free_fn at creation time allows the list to
 * release deep resources when elements are discarded (pop with NULL out,
 * remove with NULL out, destroy).
 *
 * ### Thread-safety
 * No internal locking.  Concurrent access requires external synchronisation.
 */
typedef struct cdsa_slist      cdsa_slist_t;
typedef struct cdsa_slist_node cdsa_slist_node_t;

/* ── Create / destroy ────────────────────────────────────────────────────── */

/**
 * @brief Create an empty singly linked list.
 *
 * @param elem_size  Size of each element in bytes.  Must be > 0.
 * @param cmp_fn     Comparator used by cdsa_slist_find() and
 *                   cdsa_slist_remove().  May be NULL if those operations
 *                   are never called.
 * @param free_fn    Destructor called when an element is discarded without an
 *                   output buffer, or when the list is destroyed.  May be NULL.
 * @param out        Receives the new list pointer.  Must not be NULL.
 * @return           CDSA_OK on success; CDSA_ERR_INVAL if elem_size == 0 or
 *                   out is NULL; CDSA_ERR_NOMEM if allocation fails.
 * @complexity       O(1) time, O(1) space.
 */
cdsa_status_t cdsa_slist_create(size_t elem_size, cdsa_cmp_fn cmp_fn,
                                 cdsa_free_fn free_fn, cdsa_slist_t **out);

/**
 * @brief Destroy the list and all its nodes.
 *
 * If free_fn was provided, it is called on every remaining element before the
 * node is freed.  Passing NULL is a no-op.
 *
 * @param list  List to destroy, or NULL.
 * @complexity  O(n) if free_fn != NULL; O(n) to free nodes.
 */
void cdsa_slist_destroy(cdsa_slist_t *list);

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
cdsa_status_t cdsa_slist_push_front(cdsa_slist_t *list, const void *elem);

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
cdsa_status_t cdsa_slist_pop_front(cdsa_slist_t *list, void *out);

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
cdsa_status_t cdsa_slist_push_back(cdsa_slist_t *list, const void *elem);

/**
 * @brief Remove and optionally return the back element.
 *
 * Same out / free_fn semantics as cdsa_slist_pop_front().
 *
 * @param list  Source list.  Must not be NULL.
 * @param out   Destination buffer (at least elem_size bytes), or NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if list is NULL;
 *              CDSA_ERR_EMPTY if the list is empty.
 * @complexity  O(n) time (must walk to find second-to-last node).
 */
cdsa_status_t cdsa_slist_pop_back(cdsa_slist_t *list, void *out);

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
cdsa_status_t cdsa_slist_peek_front(const cdsa_slist_t *list, void *out);

/**
 * @brief Copy the back element into @p out without removing it.
 *
 * @param list  Source list.  Must not be NULL.
 * @param out   Destination buffer (at least elem_size bytes).  Must not be NULL.
 * @return      CDSA_OK; CDSA_ERR_INVAL if list or out is NULL;
 *              CDSA_ERR_EMPTY if the list is empty.
 * @complexity  O(1) time.
 */
cdsa_status_t cdsa_slist_peek_back(const cdsa_slist_t *list, void *out);

/* ── Find / remove ───────────────────────────────────────────────────────── */

/**
 * @brief Return a pointer to the first node whose element compares equal to
 *        @p key (using the list's cmp_fn).
 *
 * The returned node pointer is owned by the list and remains valid until
 * the node is removed or the list is destroyed.
 *
 * @param list      Source list.  Must not be NULL.
 * @param key       Key to search for.  Must not be NULL.
 * @param node_out  Receives a pointer to the matching node.  Must not be NULL.
 * @return          CDSA_OK on success; CDSA_ERR_INVAL if any arg is NULL or
 *                  cmp_fn was not set at creation; CDSA_ERR_NOTFOUND if no
 *                  element matches.
 * @complexity      O(n) time.
 */
cdsa_status_t cdsa_slist_find(const cdsa_slist_t *list, const void *key,
                               cdsa_slist_node_t **node_out);

/**
 * @brief Remove the first element that compares equal to @p key.
 *
 * Same out / free_fn semantics as cdsa_slist_pop_front().
 *
 * @param list  Target list.  Must not be NULL.
 * @param key   Key to remove.  Must not be NULL.
 * @param out   Destination buffer (at least elem_size bytes), or NULL.
 * @return      CDSA_OK on success; CDSA_ERR_INVAL if list or key is NULL or
 *              cmp_fn was not set at creation; CDSA_ERR_NOTFOUND if no
 *              element matches.
 * @complexity  O(n) time.
 */
cdsa_status_t cdsa_slist_remove(cdsa_slist_t *list, const void *key,
                                 void *out);

/* ── Accessors ───────────────────────────────────────────────────────────── */

/**
 * @brief Return the number of elements in the list.
 *
 * @param list  Source list (NULL returns 0).
 * @return      Element count.
 * @complexity  O(1) time.
 */
size_t cdsa_slist_len(const cdsa_slist_t *list);

/* ── Iteration ───────────────────────────────────────────────────────────── */

/**
 * @brief Return a pointer to the first real node (after the sentinel), or NULL
 *        if the list is empty.
 *
 * @param list  Source list (NULL returns NULL).
 * @return      First node, or NULL.
 * @complexity  O(1) time.
 */
cdsa_slist_node_t *cdsa_slist_head(const cdsa_slist_t *list);

/**
 * @brief Return the successor of @p node, or NULL if @p node is the last.
 *
 * @param node  Current node (NULL returns NULL).
 * @return      Next node, or NULL.
 * @complexity  O(1) time.
 */
cdsa_slist_node_t *cdsa_slist_node_next(cdsa_slist_node_t *node);

/**
 * @brief Return a pointer to the element data stored in @p node.
 *
 * The returned pointer is valid for the lifetime of the node.
 *
 * @param node  Node whose data to access.  Must not be NULL.
 * @return      Pointer to elem_size bytes of element data.
 * @complexity  O(1) time.
 */
void *cdsa_slist_node_data(cdsa_slist_node_t *node);

/* ── Iteration macro ─────────────────────────────────────────────────────── */

/**
 * @brief Iterate over every node of the list front-to-back.
 *
 * @p node_ is set to each @c cdsa_slist_node_t* in turn.  Declare it in the
 * enclosing scope before using this macro.  Modifying the list (push, pop,
 * remove) during iteration produces undefined behaviour.  @c break and
 * @c continue work as expected.
 *
 * Example:
 * @code
 *   cdsa_slist_node_t *n;
 *   CDSA_SLIST_FOREACH(list, n) {
 *       printf("%d\n", *(int *)cdsa_slist_node_data(n));
 *   }
 * @endcode
 *
 * @param list_  @c cdsa_slist_t* to iterate over.
 * @param node_  @c cdsa_slist_node_t* lvalue set to each node.
 * @complexity   O(1) per node; O(n) for a complete traversal.
 */
#define CDSA_SLIST_FOREACH(list_, node_)                              \
    for ((node_) = cdsa_slist_head(list_);                            \
         (node_) != NULL;                                             \
         (node_) = cdsa_slist_node_next(node_))

#endif /* CDSA_SLIST_H */
