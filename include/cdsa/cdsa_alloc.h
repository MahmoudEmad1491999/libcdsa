#ifndef CDSA_ALLOC_H
#define CDSA_ALLOC_H

#include <stddef.h>

/**
 * @file cdsa_alloc.h
 * @brief Process-wide allocator hook and internal allocation wrappers.
 *
 * All library allocations go through cdsa_malloc / cdsa_realloc / cdsa_free
 * rather than the C library directly.  Callers can replace the allocator at
 * start-up (e.g. for arenas or pools) and tests can inject failure via
 * cdsa_alloc_fail_after().
 *
 * Thread-safety: cdsa_allocator_set() and cdsa_alloc_fail_after() are NOT
 * thread-safe and must be called before any concurrent library activity.
 * The wrappers themselves (cdsa_malloc / cdsa_realloc / cdsa_free) are safe
 * iff the installed allocator is safe.
 */

/** malloc-compatible hook: allocate @p size bytes; returns NULL on failure. */
typedef void *(*cdsa_malloc_hook_t)(size_t size, void *ctx);

/** realloc-compatible hook: resize @p ptr to @p size; returns NULL on failure. */
typedef void *(*cdsa_realloc_hook_t)(void *ptr, size_t size, void *ctx);

/** free-compatible hook: release a block obtained from the same allocator. */
typedef void (*cdsa_free_hook_t)(void *ptr, void *ctx);

/**
 * @brief Describes a complete replacement allocator.
 *
 * All three hooks must be non-NULL when passed to cdsa_allocator_set().
 * @p ctx is forwarded to every callback unchanged.
 */
typedef struct cdsa_allocator {
    cdsa_malloc_hook_t  malloc_fn;
    cdsa_realloc_hook_t realloc_fn;
    cdsa_free_hook_t    free_fn;
    void               *ctx;
} cdsa_allocator_t;

/**
 * @brief Replace the process-wide allocator.
 *
 * Passing NULL restores the default libc allocator.
 * Not thread-safe; call before any concurrent library use.
 *
 * @param alloc  Allocator to install, or NULL to restore the default.
 * @complexity O(1).
 */
void cdsa_allocator_set(const cdsa_allocator_t *alloc);

/**
 * @brief Return a pointer to the currently active allocator.
 *
 * @return Non-NULL pointer to the active cdsa_allocator_t.
 * @complexity O(1).
 */
const cdsa_allocator_t *cdsa_allocator_get(void);

/**
 * @brief Allocate @p size bytes through the active allocator.
 *
 * Returns NULL on failure; behaviour is undefined for @p size == 0.
 *
 * @param size  Number of bytes to allocate.
 * @return      Pointer to the new block, or NULL on failure.
 * @complexity O(1) amortised for the default allocator.
 */
void *cdsa_malloc(size_t size);

/**
 * @brief Resize @p ptr to @p size bytes through the active allocator.
 *
 * If @p ptr is NULL, behaves like cdsa_malloc(@p size).
 * On failure returns NULL and leaves @p ptr valid (not freed).
 *
 * @param ptr   Block to resize, or NULL.
 * @param size  New size in bytes.
 * @return      Pointer to the resized block, or NULL on failure.
 * @complexity O(1) amortised for the default allocator.
 */
void *cdsa_realloc(void *ptr, size_t size);

/**
 * @brief Free a block obtained from cdsa_malloc() or cdsa_realloc().
 *
 * Passing NULL is a no-op.
 *
 * @param ptr  Block to free, or NULL.
 * @complexity O(1) amortised for the default allocator.
 */
void cdsa_free(void *ptr);

/* ── Test / debug helper ──────────────────────────────────────────────────── */

/**
 * @brief Cause the next @p n calls to cdsa_malloc() or cdsa_realloc() to
 *        return NULL, simulating allocation failure.
 *
 * After @p n failures the allocator resumes normal operation automatically.
 * Pass 0 to cancel any pending failures.
 * Not thread-safe; intended for single-threaded unit tests only.
 *
 * @param n  Number of allocations to fail.
 * @complexity O(1).
 */
void cdsa_alloc_fail_after(size_t n);

#endif /* CDSA_ALLOC_H */
