#ifndef CDSA_VEC_H
#define CDSA_VEC_H

#include <stddef.h>

#include "cdsa/cdsa_callbacks.h"
#include "cdsa/cdsa_status.h"

/**
 * @file cdsa_vec.h
 * @brief Dynamic array (vector) with generic elements and geometric growth.
 *
 * cdsa_vec_t is an opaque, heap-allocated array that grows automatically.
 * Capacity doubles whenever the array is full (geometric growth, factor 2),
 * giving amortised O(1) push cost.  All elements are stored by value: the
 * vector owns copies of elem_size bytes per slot.
 *
 * ### Ownership model
 * On push, elem_size bytes are copied from the caller's buffer into the
 * vector.  The vector does not deep-copy pointer members; supply a
 * cdsa_free_fn to release them on pop or destroy.
 *
 * ### Thread-safety
 * No internal locking.  Concurrent access requires external synchronisation.
 */
typedef struct cdsa_vec cdsa_vec_t;

/**
 * @brief Create an empty vector.
 *
 * No buffer is allocated until the first push.
 *
 * @param elem_size  Size of each element in bytes.  Must be > 0.
 * @param free_fn    Destructor called when an element is removed without an
 *                   output buffer, or when the vector is destroyed.  May be
 *                   NULL (elements are discarded without clean-up).
 * @param out        Receives the new vector pointer.  Must not be NULL.
 * @return           CDSA_OK on success; CDSA_ERR_INVAL if elem_size == 0 or
 *                   out is NULL; CDSA_ERR_NOMEM if the struct allocation fails.
 * @complexity       O(1) time, O(1) space.
 */
cdsa_status_t cdsa_vec_create(size_t elem_size, cdsa_free_fn free_fn,
                               cdsa_vec_t **out);

/**
 * @brief Destroy the vector and all its elements.
 *
 * If free_fn was provided at creation time, it is called on every remaining
 * element.  The internal buffer and the vector struct are then freed.
 * Passing NULL is a no-op.
 *
 * @param v  Vector to destroy, or NULL.
 * @complexity O(n) if free_fn != NULL, O(1) otherwise.
 */
void cdsa_vec_destroy(cdsa_vec_t *v);

/**
 * @brief Append a copy of elem to the back of the vector.
 *
 * If the vector is full the internal buffer is reallocated with doubled
 * capacity before the element is appended.  On reallocation failure the
 * vector is left unchanged.
 *
 * @param v     Target vector.  Must not be NULL.
 * @param elem  Source buffer of at least elem_size bytes.  Must not be NULL.
 * @return      CDSA_OK on success; CDSA_ERR_INVAL if v or elem is NULL;
 *              CDSA_ERR_NOMEM if reallocation fails.
 * @complexity  O(1) amortised; O(n) worst-case on reallocation.
 */
cdsa_status_t cdsa_vec_push(cdsa_vec_t *v, const void *elem);

/**
 * @brief Remove the last element from the vector.
 *
 * If @p out is non-NULL, elem_size bytes are copied into @p out and free_fn
 * is NOT called (the caller takes ownership of the element).  If @p out is
 * NULL, free_fn is called on the element (if set) before it is discarded.
 *
 * @param v    Source vector.  Must not be NULL.
 * @param out  Destination buffer of at least elem_size bytes, or NULL.
 * @return     CDSA_OK on success; CDSA_ERR_INVAL if v is NULL;
 *             CDSA_ERR_EMPTY if the vector is empty.
 * @complexity O(1) time.
 */
cdsa_status_t cdsa_vec_pop(cdsa_vec_t *v, void *out);

/**
 * @brief Obtain a pointer to the element at @p index.
 *
 * The returned pointer points directly into the vector's internal buffer.
 * It is invalidated by any operation that may reallocate the buffer (push).
 *
 * @param v        Source vector.  Must not be NULL.
 * @param index    Zero-based element index.
 * @param elem_out Receives a pointer to the element slot.  Must not be NULL.
 * @return         CDSA_OK on success; CDSA_ERR_INVAL if v or elem_out is NULL;
 *                 CDSA_ERR_RANGE if index >= len.
 * @complexity     O(1) time.
 */
cdsa_status_t cdsa_vec_get(const cdsa_vec_t *v, size_t index, void **elem_out);

/**
 * @brief Overwrite the element at @p index with a copy of @p elem.
 *
 * If free_fn was provided at creation time, it is called on the old element
 * before the new value is written.
 *
 * @param v      Target vector.  Must not be NULL.
 * @param index  Zero-based element index.
 * @param elem   Source buffer of at least elem_size bytes.  Must not be NULL.
 * @return       CDSA_OK on success; CDSA_ERR_INVAL if v or elem is NULL;
 *               CDSA_ERR_RANGE if index >= len.
 * @complexity   O(1) time.
 */
cdsa_status_t cdsa_vec_set(cdsa_vec_t *v, size_t index, const void *elem);

/**
 * @brief Return the number of elements currently stored in the vector.
 *
 * @param v  Source vector (NULL returns 0).
 * @return   Current element count.
 * @complexity O(1) time.
 */
size_t cdsa_vec_len(const cdsa_vec_t *v);

/**
 * @brief Return the current capacity of the vector.
 *
 * The capacity is the number of elements the internal buffer can hold without
 * reallocation.
 *
 * @param v  Source vector (NULL returns 0).
 * @return   Current capacity in elements.
 * @complexity O(1) time.
 */
size_t cdsa_vec_cap(const cdsa_vec_t *v);

#endif /* CDSA_VEC_H */
