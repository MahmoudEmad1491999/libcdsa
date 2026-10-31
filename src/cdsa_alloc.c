#include "cdsa/cdsa_alloc.h"

#include <stdlib.h>

/* ── Default libc allocator ───────────────────────────────────────────────── */

static void *default_malloc(size_t size, void *ctx)
{
    (void)ctx;
    return malloc(size);
}

static void *default_realloc(void *ptr, size_t size, void *ctx)
{
    (void)ctx;
    return realloc(ptr, size);
}

static void default_free(void *ptr, void *ctx)
{
    (void)ctx;
    free(ptr);
}

static const cdsa_allocator_t g_default_alloc = {
    default_malloc,
    default_realloc,
    default_free,
    NULL
};

/* Active allocator — starts as the default, can be swapped by callers. */
static cdsa_allocator_t g_alloc = {
    default_malloc,
    default_realloc,
    default_free,
    NULL
};

/* ── Fail counter (test/debug helper) ────────────────────────────────────── */

/* Number of upcoming cdsa_malloc/cdsa_realloc calls that will return NULL. */
static size_t g_fail_countdown = 0;

/* ── Public API ───────────────────────────────────────────────────────────── */

void cdsa_allocator_set(const cdsa_allocator_t *alloc)
{
    if (alloc == NULL) {
        g_alloc = g_default_alloc;
    } else {
        g_alloc = *alloc;
    }
}

const cdsa_allocator_t *cdsa_allocator_get(void)
{
    return &g_alloc;
}

void *cdsa_malloc(size_t size)
{
    if (g_fail_countdown > 0) {
        --g_fail_countdown;
        return NULL;
    }
    return g_alloc.malloc_fn(size, g_alloc.ctx);
}

void *cdsa_realloc(void *ptr, size_t size)
{
    if (g_fail_countdown > 0) {
        --g_fail_countdown;
        return NULL;
    }
    return g_alloc.realloc_fn(ptr, size, g_alloc.ctx);
}

void cdsa_free(void *ptr)
{
    g_alloc.free_fn(ptr, g_alloc.ctx);
}

void cdsa_alloc_fail_after(size_t n)
{
    g_fail_countdown = n;
}
