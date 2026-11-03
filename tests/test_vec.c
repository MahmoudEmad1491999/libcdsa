#include "check.h"
#include "cdsa/cdsa_vec.h"
#include "cdsa/cdsa_alloc.h"
#include "cdsa/cdsa_status.h"

#include <stddef.h>
#include <string.h>

/* ── Helpers ─────────────────────────────────────────────────────────────── */

/* Counter incremented by the test free_fn below. */
static size_t g_free_count = 0;

static void counting_free(void *elem)
{
    (void)elem;
    ++g_free_count;
}

/* Reset allocation state and free counter between tests. */
static void reset(void)
{
    cdsa_alloc_fail_after(0);
    cdsa_allocator_set(NULL);
    g_free_count = 0;
}

/* ── cdsa_vec_create ─────────────────────────────────────────────────────── */

static void test_create_basic(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    CHECK_NOTNULL(v);
    CHECK_EQ(cdsa_vec_len(v), (size_t)0);
    CHECK_EQ(cdsa_vec_cap(v), (size_t)0);
    cdsa_vec_destroy(v);
}

static void test_create_null_out(void)
{
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, NULL), CDSA_ERR_INVAL);
}

static void test_create_zero_elem_size(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(0, NULL, &v), CDSA_ERR_INVAL);
    CHECK_NULL(v);
}

static void test_create_alloc_failure(void)
{
    cdsa_vec_t *v = NULL;
    cdsa_alloc_fail_after(1);
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_ERR_NOMEM);
    CHECK_NULL(v);
    reset();
}

/* ── cdsa_vec_destroy ────────────────────────────────────────────────────── */

static void test_destroy_null(void)
{
    /* Must not crash. */
    cdsa_vec_destroy(NULL);
}

static void test_destroy_calls_free_fn(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), counting_free, &v), CDSA_OK);
    int val = 1;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    val = 2;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    val = 3;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);

    g_free_count = 0;
    cdsa_vec_destroy(v);
    CHECK_EQ(g_free_count, (size_t)3);
}

static void test_destroy_no_free_fn(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 99;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    /* Must not crash even without free_fn. */
    cdsa_vec_destroy(v);
}

/* ── cdsa_vec_push ───────────────────────────────────────────────────────── */

static void test_push_basic(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);

    for (int i = 0; i < 10; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
        CHECK_EQ(cdsa_vec_len(v), (size_t)(i + 1));
    }

    void *elem = NULL;
    for (int i = 0; i < 10; ++i) {
        CHECK_EQ(cdsa_vec_get(v, (size_t)i, &elem), CDSA_OK);
        CHECK_EQ(*(int *)elem, i);
    }

    cdsa_vec_destroy(v);
}

static void test_push_null_vec(void)
{
    int val = 1;
    CHECK_EQ(cdsa_vec_push(NULL, &val), CDSA_ERR_INVAL);
}

static void test_push_null_elem(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    CHECK_EQ(cdsa_vec_push(v, NULL), CDSA_ERR_INVAL);
    cdsa_vec_destroy(v);
}

static void test_push_alloc_failure(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);

    /* First push triggers the initial buffer allocation — make it fail. */
    cdsa_alloc_fail_after(1);
    int val = 42;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_ERR_NOMEM);
    CHECK_EQ(cdsa_vec_len(v), (size_t)0); /* vector unchanged */
    reset();

    cdsa_vec_destroy(v);
}

/* ── cdsa_vec_pop ────────────────────────────────────────────────────────── */

static void test_pop_basic(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);

    int in  = 77;
    CHECK_EQ(cdsa_vec_push(v, &in), CDSA_OK);

    int out = 0;
    CHECK_EQ(cdsa_vec_pop(v, &out), CDSA_OK);
    CHECK_EQ(out, 77);
    CHECK_EQ(cdsa_vec_len(v), (size_t)0);

    cdsa_vec_destroy(v);
}

static void test_pop_lifo_order(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);

    for (int i = 1; i <= 5; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }
    for (int expected = 5; expected >= 1; --expected) {
        int out = 0;
        CHECK_EQ(cdsa_vec_pop(v, &out), CDSA_OK);
        CHECK_EQ(out, expected);
    }

    cdsa_vec_destroy(v);
}

static void test_pop_empty(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    CHECK_EQ(cdsa_vec_pop(v, NULL), CDSA_ERR_EMPTY);
    cdsa_vec_destroy(v);
}

static void test_pop_null_vec(void)
{
    CHECK_EQ(cdsa_vec_pop(NULL, NULL), CDSA_ERR_INVAL);
}

static void test_pop_null_out_calls_free_fn(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), counting_free, &v), CDSA_OK);

    int val = 10;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);

    g_free_count = 0;
    CHECK_EQ(cdsa_vec_pop(v, NULL), CDSA_OK); /* out == NULL → calls free_fn */
    CHECK_EQ(g_free_count, (size_t)1);

    cdsa_vec_destroy(v);
}

static void test_pop_nonnull_out_no_free_fn(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), counting_free, &v), CDSA_OK);

    int val = 55;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);

    g_free_count = 0;
    int out = 0;
    CHECK_EQ(cdsa_vec_pop(v, &out), CDSA_OK); /* out != NULL → no free_fn */
    CHECK_EQ(g_free_count, (size_t)0);
    CHECK_EQ(out, 55);

    cdsa_vec_destroy(v);
}

/* ── cdsa_vec_get ────────────────────────────────────────────────────────── */

static void test_get_basic(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);

    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }

    void *p = NULL;
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(cdsa_vec_get(v, (size_t)i, &p), CDSA_OK);
        CHECK_NOTNULL(p);
        CHECK_EQ(*(int *)p, i);
    }

    cdsa_vec_destroy(v);
}

static void test_get_null_vec(void)
{
    void *p = NULL;
    CHECK_EQ(cdsa_vec_get(NULL, 0, &p), CDSA_ERR_INVAL);
}

static void test_get_null_out(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 1;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_get(v, 0, NULL), CDSA_ERR_INVAL);
    cdsa_vec_destroy(v);
}

static void test_get_out_of_range(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    void *p = NULL;
    CHECK_EQ(cdsa_vec_get(v, 0, &p), CDSA_ERR_RANGE);  /* empty */
    int val = 1;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_get(v, 1, &p), CDSA_ERR_RANGE);  /* one past end */
    cdsa_vec_destroy(v);
}

/* ── cdsa_vec_set ────────────────────────────────────────────────────────── */

static void test_set_basic(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 10;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);

    int replacement = 99;
    CHECK_EQ(cdsa_vec_set(v, 0, &replacement), CDSA_OK);

    void *p = NULL;
    CHECK_EQ(cdsa_vec_get(v, 0, &p), CDSA_OK);
    CHECK_EQ(*(int *)p, 99);

    cdsa_vec_destroy(v);
}

static void test_set_null_vec(void)
{
    int val = 1;
    CHECK_EQ(cdsa_vec_set(NULL, 0, &val), CDSA_ERR_INVAL);
}

static void test_set_null_elem(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 1;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_set(v, 0, NULL), CDSA_ERR_INVAL);
    cdsa_vec_destroy(v);
}

static void test_set_out_of_range(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 1;
    CHECK_EQ(cdsa_vec_set(v, 0, &val), CDSA_ERR_RANGE);  /* empty */
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_set(v, 1, &val), CDSA_ERR_RANGE);  /* one past end */
    cdsa_vec_destroy(v);
}

static void test_set_calls_free_fn(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), counting_free, &v), CDSA_OK);
    int val = 7;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);

    g_free_count = 0;
    int replacement = 8;
    CHECK_EQ(cdsa_vec_set(v, 0, &replacement), CDSA_OK);
    CHECK_EQ(g_free_count, (size_t)1);  /* free_fn called on old element */

    void *p = NULL;
    CHECK_EQ(cdsa_vec_get(v, 0, &p), CDSA_OK);
    CHECK_EQ(*(int *)p, 8);

    cdsa_vec_destroy(v);
}

/* ── cdsa_vec_len / cdsa_vec_cap ─────────────────────────────────────────── */

static void test_len_null(void)
{
    CHECK_EQ(cdsa_vec_len(NULL), (size_t)0);
}

static void test_cap_null(void)
{
    CHECK_EQ(cdsa_vec_cap(NULL), (size_t)0);
}

/* ── Geometric growth ────────────────────────────────────────────────────── */

static void test_geometric_growth(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);

    /* Before any push: capacity is 0 (no allocation). */
    CHECK_EQ(cdsa_vec_cap(v), (size_t)0);

    /* Push 1st element — triggers first allocation (INITIAL_CAP = 4). */
    int val = 0;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    size_t cap_after_1 = cdsa_vec_cap(v);
    CHECK_GT(cap_after_1, (size_t)0);

    /* Fill to capacity without triggering a realloc. */
    while (cdsa_vec_len(v) < cap_after_1) {
        CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    }
    CHECK_EQ(cdsa_vec_cap(v), cap_after_1);

    /* One more push — must double the capacity. */
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_cap(v), cap_after_1 * (size_t)2);

    cdsa_vec_destroy(v);
}

/* ── Push grow alloc failure leaves vector intact ───────────────────────── */

static void test_push_grow_failure_atomic(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);

    /* Fill to the initial capacity so the next push triggers a realloc. */
    int val = 1;
    /* Push first element to find initial cap. */
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    size_t initial_cap = cdsa_vec_cap(v);
    while (cdsa_vec_len(v) < initial_cap) {
        CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    }
    size_t len_before = cdsa_vec_len(v);

    /* Make the realloc fail. */
    cdsa_alloc_fail_after(1);
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_ERR_NOMEM);

    /* Vector must be completely unchanged. */
    CHECK_EQ(cdsa_vec_len(v), len_before);
    CHECK_EQ(cdsa_vec_cap(v), initial_cap);
    reset();

    cdsa_vec_destroy(v);
}

/* ── One-element edge cases ──────────────────────────────────────────────── */

static void test_push_and_pop_single_element(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);

    int in = 42;
    CHECK_EQ(cdsa_vec_push(v, &in), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), (size_t)1);

    int out = 0;
    CHECK_EQ(cdsa_vec_pop(v, &out), CDSA_OK);
    CHECK_EQ(out, 42);
    CHECK_EQ(cdsa_vec_len(v), (size_t)0);

    /* Pop again: must return CDSA_ERR_EMPTY. */
    CHECK_EQ(cdsa_vec_pop(v, &out), CDSA_ERR_EMPTY);

    cdsa_vec_destroy(v);
}

/* ── Large element size (struct) ─────────────────────────────────────────── */

static void test_large_elem_size(void)
{
    typedef struct { int x; int y; char tag[8]; } point_t;

    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(point_t), NULL, &v), CDSA_OK);

    point_t pts[3] = {
        {1, 2, "a"},
        {3, 4, "b"},
        {5, 6, "c"},
    };

    for (int i = 0; i < 3; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &pts[i]), CDSA_OK);
    }

    for (int i = 0; i < 3; ++i) {
        void *p = NULL;
        CHECK_EQ(cdsa_vec_get(v, (size_t)i, &p), CDSA_OK);
        point_t *got = (point_t *)p;
        CHECK_EQ(got->x, pts[i].x);
        CHECK_EQ(got->y, pts[i].y);
        CHECK_EQ(strcmp(got->tag, pts[i].tag), 0);
    }

    cdsa_vec_destroy(v);
}

/* ── Test registry ───────────────────────────────────────────────────────── */

static const cdsa_test_entry_t tests[] = {
    /* create */
    CDSA_T(test_create_basic),
    CDSA_T(test_create_null_out),
    CDSA_T(test_create_zero_elem_size),
    CDSA_T(test_create_alloc_failure),
    /* destroy */
    CDSA_T(test_destroy_null),
    CDSA_T(test_destroy_calls_free_fn),
    CDSA_T(test_destroy_no_free_fn),
    /* push */
    CDSA_T(test_push_basic),
    CDSA_T(test_push_null_vec),
    CDSA_T(test_push_null_elem),
    CDSA_T(test_push_alloc_failure),
    /* pop */
    CDSA_T(test_pop_basic),
    CDSA_T(test_pop_lifo_order),
    CDSA_T(test_pop_empty),
    CDSA_T(test_pop_null_vec),
    CDSA_T(test_pop_null_out_calls_free_fn),
    CDSA_T(test_pop_nonnull_out_no_free_fn),
    /* get */
    CDSA_T(test_get_basic),
    CDSA_T(test_get_null_vec),
    CDSA_T(test_get_null_out),
    CDSA_T(test_get_out_of_range),
    /* set */
    CDSA_T(test_set_basic),
    CDSA_T(test_set_null_vec),
    CDSA_T(test_set_null_elem),
    CDSA_T(test_set_out_of_range),
    CDSA_T(test_set_calls_free_fn),
    /* accessors */
    CDSA_T(test_len_null),
    CDSA_T(test_cap_null),
    /* geometric growth */
    CDSA_T(test_geometric_growth),
    CDSA_T(test_push_grow_failure_atomic),
    /* edge cases */
    CDSA_T(test_push_and_pop_single_element),
    CDSA_T(test_large_elem_size),
};

int main(void)
{
    return cdsa_run_suite("cdsa_vec", tests, CDSA_ARRAY_LEN(tests));
}
