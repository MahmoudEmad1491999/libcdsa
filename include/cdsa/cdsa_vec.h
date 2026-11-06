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

/**
 * @brief Insert a copy of @p elem at position @p index, shifting later elements
 *        right by one.
 *
 * Valid indices are [0, len]: inserting at @p index == len is equivalent to a
 * push_back.  If the vector is full, the buffer is reallocated with the same
 * doubling policy as push_back; on failure the vector is left unchanged.
 *
 * @param v      Target vector.  Must not be NULL.
 * @param index  Insertion point (0-based).  Must be <= len.
 * @param elem   Source buffer of at least elem_size bytes.  Must not be NULL.
 * @return       CDSA_OK on success; CDSA_ERR_INVAL if v or elem is NULL;
 *               CDSA_ERR_RANGE if index > len;
 *               CDSA_ERR_NOMEM if reallocation fails.
 * @complexity   O(n) time (shift); O(1) amortised space.
 */
cdsa_status_t cdsa_vec_insert(cdsa_vec_t *v, size_t index, const void *elem);

/**
 * @brief Remove the element at @p index, shifting later elements left by one.
 *
 * If @p out is non-NULL, elem_size bytes are copied into @p out and free_fn
 * is NOT called (the caller takes ownership).  If @p out is NULL and free_fn
 * was provided, free_fn is called on the removed element.
 *
 * @param v      Source vector.  Must not be NULL.
 * @param index  Zero-based index of the element to remove.
 * @param out    Destination buffer of at least elem_size bytes, or NULL.
 * @return       CDSA_OK on success; CDSA_ERR_INVAL if v is NULL;
 *               CDSA_ERR_RANGE if index >= len (including the empty case).
 * @complexity   O(n) time (shift).
 */
cdsa_status_t cdsa_vec_erase(cdsa_vec_t *v, size_t index, void *out);

/**
 * @brief Ensure the vector can hold at least @p min_cap elements without
 *        reallocation.
 *
 * If the current capacity is already >= @p min_cap this is a no-op.
 * Otherwise the buffer is reallocated to exactly @p min_cap elements.
 * On failure the vector is left unchanged.
 *
 * @param v        Target vector.  Must not be NULL.
 * @param min_cap  Required minimum capacity in elements.
 * @return         CDSA_OK on success; CDSA_ERR_INVAL if v is NULL;
 *                 CDSA_ERR_NOMEM if reallocation fails or min_cap would
 *                 overflow size_t when multiplied by elem_size.
 * @complexity     O(n) on reallocation; O(1) if already sufficient.
 */
cdsa_status_t cdsa_vec_reserve(cdsa_vec_t *v, size_t min_cap);

/**
 * @brief Reduce the capacity to exactly @p len, releasing unused memory.
 *
 * If len == cap, this is a no-op.  If len == 0 the data buffer is freed
 * entirely and cap becomes 0.  On reallocation failure the vector is left
 * unchanged and CDSA_ERR_NOMEM is returned.
 *
 * @param v  Target vector.  Must not be NULL.
 * @return   CDSA_OK on success; CDSA_ERR_INVAL if v is NULL;
 *           CDSA_ERR_NOMEM if reallocation fails.
 * @complexity O(n) on reallocation; O(1) if already minimal.
 */
cdsa_status_t cdsa_vec_shrink_to_fit(cdsa_vec_t *v);

/**
 * @brief Remove all elements without releasing the internal buffer.
 *
 * If free_fn was provided at creation time, it is called on every element
 * before the length is reset to zero.  The capacity is preserved so that
 * subsequent pushes do not reallocate.
 *
 * Passing NULL is a no-op.
 *
 * @param v  Target vector, or NULL.
 * @complexity O(n) if free_fn != NULL; O(1) otherwise.
 */
void cdsa_vec_clear(cdsa_vec_t *v);

/* ── Iteration, swap, reverse ─────────────────────────────────────────────── */

/**
 * @brief Return a pointer to the first element of the internal buffer.
 *
 * Returns NULL when @p v is NULL or when no buffer has been allocated yet
 * (empty vector that has never been push'd or reserve'd).  The pointer is
 * invalidated by any operation that may reallocate the buffer (push, insert,
 * reserve).  Callers are responsible for staying within [0, len) bounds.
 *
 * Useful for passing the buffer to @c qsort / @c bsearch, or for fast
 * index-based loops when bounds are already known.
 *
 * @param v  Source vector, or NULL.
 * @return   Pointer to the first element slot, or NULL.
 * @complexity O(1) time.
 */
void *cdsa_vec_data(cdsa_vec_t *v);

/**
 * @brief Return the size in bytes of one element stored in the vector.
 *
 * @param v  Source vector (NULL returns 0).
 * @return   Element size in bytes, or 0 if @p v is NULL.
 * @complexity O(1) time.
 */
size_t cdsa_vec_elem_size(const cdsa_vec_t *v);

/**
 * @brief Swap the elements at indices @p i and @p j in-place.
 *
 * A temporary allocation of elem_size bytes is made; on failure the vector
 * is left unchanged.  Swapping an index with itself (@p i == @p j) is a
 * no-op and returns CDSA_OK without allocating.
 *
 * @param v  Target vector.  Must not be NULL.
 * @param i  First element index (0-based, must be < len).
 * @param j  Second element index (0-based, must be < len).
 * @return   CDSA_OK on success; CDSA_ERR_INVAL if @p v is NULL;
 *           CDSA_ERR_RANGE if @p i or @p j >= len;
 *           CDSA_ERR_NOMEM if the temporary buffer allocation fails.
 * @complexity O(elem_size) time; O(elem_size) auxiliary space.
 */
cdsa_status_t cdsa_vec_swap(cdsa_vec_t *v, size_t i, size_t j);

/**
 * @brief Reverse all elements in the vector in-place.
 *
 * A single temporary allocation of elem_size bytes is made; on failure the
 * vector is left completely unchanged.  An empty or single-element vector is
 * a no-op and returns CDSA_OK without allocating.
 *
 * @param v  Target vector.  Must not be NULL.
 * @return   CDSA_OK on success; CDSA_ERR_INVAL if @p v is NULL;
 *           CDSA_ERR_NOMEM if the temporary buffer allocation fails.
 * @complexity O(n) time; O(elem_size) auxiliary space.
 */
cdsa_status_t cdsa_vec_reverse(cdsa_vec_t *v);

/* ── Iteration macro ──────────────────────────────────────────────────────── */

/**
 * @brief Token-pasting helpers that produce unique identifiers per call site.
 * @internal
 */
#ifndef CDSA_CONCAT_
#  define CDSA_CONCAT_IMPL_(a_, b_) a_##b_
#  define CDSA_CONCAT_(a_, b_)      CDSA_CONCAT_IMPL_(a_, b_)
#endif

/**
 * @brief Iterate over every element of a vector front-to-back.
 *
 * @p ptr_ is set to a @c void* pointing directly into the vector's internal
 * buffer before each loop body execution.  Declare @p ptr_ as @c void* in
 * the enclosing scope before using this macro.
 *
 * Example:
 * @code
 *   void *p;
 *   CDSA_VEC_FOREACH(v, p) {
 *       printf("%d\n", *(int *)p);
 *   }
 * @endcode
 *
 * @p v_ must be a plain variable (not an expression with side-effects); it is
 * evaluated more than once per iteration.  Modifying the vector (push, insert,
 * erase, …) during iteration invalidates the internal buffer and produces
 * undefined behaviour.  @c break and @c continue work as expected.
 *
 * @param v_    @c cdsa_vec_t* to iterate over.
 * @param ptr_  @c void* lvalue that receives a pointer to the current element.
 * @complexity  O(1) per element; O(n) for a complete traversal.
 */
#define CDSA_VEC_FOREACH(v_, ptr_)                                             \
    for (size_t CDSA_CONCAT_(cdsa_fi_, __LINE__) = 0u;                        \
         CDSA_CONCAT_(cdsa_fi_, __LINE__) < cdsa_vec_len(v_) &&               \
             (cdsa_vec_get((v_), CDSA_CONCAT_(cdsa_fi_, __LINE__), &(ptr_))   \
                  == CDSA_OK);                                                 \
         CDSA_CONCAT_(cdsa_fi_, __LINE__)++)

#endif /* CDSA_VEC_H */
