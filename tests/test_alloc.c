#include "cdsa/cdsa_alloc.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* ── Custom allocator for injection tests ────────────────────────────────── */

static size_t g_custom_malloc_calls = 0;
static size_t g_custom_free_calls   = 0;

static void *custom_malloc(size_t size, void *ctx)
{
    (void)ctx;
    ++g_custom_malloc_calls;
    return malloc(size);
}

static void *custom_realloc(void *ptr, size_t size, void *ctx)
{
    (void)ctx;
    return realloc(ptr, size);
}

static void custom_free(void *ptr, void *ctx)
{
    (void)ctx;
    ++g_custom_free_calls;
    free(ptr);
}

/* ── Helper: restore default allocator and cancel any pending failures. ── */

static void reset_alloc_state(void)
{
    cdsa_allocator_set(NULL);
    cdsa_alloc_fail_after(0);
}

/* ── Tests ───────────────────────────────────────────────────────────────── */

static void test_default_malloc_and_free(void)
{
    void *p = cdsa_malloc(64);
    assert(p != NULL);
    memset(p, 0xAB, 64);
    cdsa_free(p);
}

static void test_default_realloc_grow(void)
{
    void *p = cdsa_malloc(16);
    assert(p != NULL);
    void *q = cdsa_realloc(p, 128);
    assert(q != NULL);
    cdsa_free(q);
}

static void test_realloc_null_ptr(void)
{
    /* realloc(NULL, size) must behave like malloc(size). */
    void *p = cdsa_realloc(NULL, 32);
    assert(p != NULL);
    cdsa_free(p);
}

static void test_free_null(void)
{
    /* cdsa_free(NULL) must be a no-op; must not crash. */
    cdsa_free(NULL);
}

static void test_allocator_get_not_null(void)
{
    const cdsa_allocator_t *a = cdsa_allocator_get();
    assert(a != NULL);
    assert(a->malloc_fn != NULL);
    assert(a->realloc_fn != NULL);
    assert(a->free_fn != NULL);
    (void)a; /* suppress unused-variable in -DNDEBUG builds */
}

static void test_custom_allocator_injection(void)
{
    cdsa_allocator_t custom = {custom_malloc, custom_realloc, custom_free, NULL};

    g_custom_malloc_calls = 0;
    g_custom_free_calls   = 0;

    cdsa_allocator_set(&custom);

    void *p = cdsa_malloc(32);
    assert(p != NULL);
    assert(g_custom_malloc_calls == 1);

    cdsa_free(p);
    assert(g_custom_free_calls == 1);

    reset_alloc_state();
}

static void test_restore_default_with_null(void)
{
    cdsa_allocator_t custom = {custom_malloc, custom_realloc, custom_free, NULL};
    cdsa_allocator_set(&custom);
    cdsa_allocator_set(NULL); /* restore default */

    g_custom_malloc_calls = 0;
    void *p = cdsa_malloc(8);
    assert(p != NULL);
    assert(g_custom_malloc_calls == 0); /* default allocator; custom not called */
    cdsa_free(p);
}

static void test_fail_after_single_malloc(void)
{
    cdsa_alloc_fail_after(1);
    void *p = cdsa_malloc(16);
    assert(p == NULL); /* must fail */
    (void)p;           /* suppress unused-variable in -DNDEBUG builds */

    void *q = cdsa_malloc(16); /* counter exhausted; must succeed */
    assert(q != NULL);
    cdsa_free(q);
}

static void test_fail_after_single_realloc(void)
{
    cdsa_alloc_fail_after(1);
    void *p = cdsa_realloc(NULL, 16);
    assert(p == NULL); /* must fail */
    (void)p;

    void *q = cdsa_realloc(NULL, 16); /* must succeed */
    assert(q != NULL);
    cdsa_free(q);
}

static void test_fail_multiple(void)
{
    cdsa_alloc_fail_after(3);
    assert(cdsa_malloc(8) == NULL);
    assert(cdsa_malloc(8) == NULL);
    assert(cdsa_malloc(8) == NULL);

    void *p = cdsa_malloc(8); /* 4th call — must succeed */
    assert(p != NULL);
    cdsa_free(p);
}

static void test_fail_zero_cancels(void)
{
    cdsa_alloc_fail_after(5);
    cdsa_alloc_fail_after(0); /* cancel */

    void *p = cdsa_malloc(8);
    assert(p != NULL);
    cdsa_free(p);
}

static void test_fail_interleaved_malloc_realloc(void)
{
    /* Both malloc and realloc consume the fail counter. */
    cdsa_alloc_fail_after(2);
    assert(cdsa_malloc(8) == NULL);          /* consumes 1 */
    assert(cdsa_realloc(NULL, 8) == NULL);   /* consumes 1 */

    void *p = cdsa_malloc(8); /* must succeed */
    assert(p != NULL);
    cdsa_free(p);
}

static void test_fail_does_not_affect_free(void)
{
    void *p = cdsa_malloc(8);
    assert(p != NULL);

    cdsa_alloc_fail_after(10); /* set fail counter high */
    cdsa_free(p);              /* free must not be affected */
    cdsa_alloc_fail_after(0);  /* reset */
}

int main(void)
{
    reset_alloc_state();

    test_default_malloc_and_free();
    test_default_realloc_grow();
    test_realloc_null_ptr();
    test_free_null();
    test_allocator_get_not_null();
    test_custom_allocator_injection();
    test_restore_default_with_null();
    test_fail_after_single_malloc();
    test_fail_after_single_realloc();
    test_fail_multiple();
    test_fail_zero_cancels();
    test_fail_interleaved_malloc_realloc();
    test_fail_does_not_affect_free();

    return EXIT_SUCCESS;
}
