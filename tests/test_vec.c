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

/* ── cdsa_vec_insert ─────────────────────────────────────────────────────── */

static void test_insert_at_end(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int a = 1, b = 2;
    CHECK_EQ(cdsa_vec_push(v, &a), CDSA_OK);
    CHECK_EQ(cdsa_vec_insert(v, 1, &b), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), (size_t)2);
    void *p = NULL;
    CHECK_EQ(cdsa_vec_get(v, 0, &p), CDSA_OK); CHECK_EQ(*(int *)p, 1);
    CHECK_EQ(cdsa_vec_get(v, 1, &p), CDSA_OK); CHECK_EQ(*(int *)p, 2);
    cdsa_vec_destroy(v);
}

static void test_insert_at_front(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    for (int i = 1; i <= 3; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }
    int zero = 0;
    CHECK_EQ(cdsa_vec_insert(v, 0, &zero), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), (size_t)4);
    void *p = NULL;
    for (int i = 0; i < 4; ++i) {
        CHECK_EQ(cdsa_vec_get(v, (size_t)i, &p), CDSA_OK);
        CHECK_EQ(*(int *)p, i);
    }
    cdsa_vec_destroy(v);
}

static void test_insert_at_middle(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int vals[4] = {1, 2, 4, 5};
    for (int i = 0; i < 4; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &vals[i]), CDSA_OK);
    }
    int three = 3;
    CHECK_EQ(cdsa_vec_insert(v, 2, &three), CDSA_OK);  /* insert 3 between 2 and 4 */
    CHECK_EQ(cdsa_vec_len(v), (size_t)5);
    void *p = NULL;
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(cdsa_vec_get(v, (size_t)i, &p), CDSA_OK);
        CHECK_EQ(*(int *)p, i + 1);
    }
    cdsa_vec_destroy(v);
}

static void test_insert_forces_grow(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 0;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    size_t initial_cap = cdsa_vec_cap(v);
    while (cdsa_vec_len(v) < initial_cap) {
        CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    }
    size_t len_before = cdsa_vec_len(v);
    CHECK_EQ(cdsa_vec_insert(v, 0, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), len_before + 1u);
    CHECK_GT(cdsa_vec_cap(v), initial_cap);
    cdsa_vec_destroy(v);
}

static void test_insert_into_empty(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 42;
    CHECK_EQ(cdsa_vec_insert(v, 0, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), (size_t)1);
    void *p = NULL;
    CHECK_EQ(cdsa_vec_get(v, 0, &p), CDSA_OK);
    CHECK_EQ(*(int *)p, 42);
    cdsa_vec_destroy(v);
}

static void test_insert_null_vec(void)
{
    int val = 1;
    CHECK_EQ(cdsa_vec_insert(NULL, 0, &val), CDSA_ERR_INVAL);
}

static void test_insert_null_elem(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    CHECK_EQ(cdsa_vec_insert(v, 0, NULL), CDSA_ERR_INVAL);
    cdsa_vec_destroy(v);
}

static void test_insert_out_of_range(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 1;
    CHECK_EQ(cdsa_vec_insert(v, 1, &val), CDSA_ERR_RANGE);  /* empty, only 0 valid */
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_insert(v, 2, &val), CDSA_ERR_RANGE);  /* len==1, only 0..1 valid */
    cdsa_vec_destroy(v);
}

static void test_insert_alloc_failure(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 1;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    size_t initial_cap = cdsa_vec_cap(v);
    while (cdsa_vec_len(v) < initial_cap) {
        CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    }
    size_t len_before = cdsa_vec_len(v);

    cdsa_alloc_fail_after(1);
    CHECK_EQ(cdsa_vec_insert(v, 0, &val), CDSA_ERR_NOMEM);
    CHECK_EQ(cdsa_vec_len(v), len_before);
    CHECK_EQ(cdsa_vec_cap(v), initial_cap);
    reset();

    cdsa_vec_destroy(v);
}

/* ── cdsa_vec_erase ──────────────────────────────────────────────────────── */

static void test_erase_front(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    for (int i = 0; i < 4; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }
    CHECK_EQ(cdsa_vec_erase(v, 0, NULL), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), (size_t)3);
    void *p = NULL;
    for (int i = 0; i < 3; ++i) {
        CHECK_EQ(cdsa_vec_get(v, (size_t)i, &p), CDSA_OK);
        CHECK_EQ(*(int *)p, i + 1);
    }
    cdsa_vec_destroy(v);
}

static void test_erase_back(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    for (int i = 0; i < 3; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }
    CHECK_EQ(cdsa_vec_erase(v, 2, NULL), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), (size_t)2);
    void *p = NULL;
    CHECK_EQ(cdsa_vec_get(v, 0, &p), CDSA_OK); CHECK_EQ(*(int *)p, 0);
    CHECK_EQ(cdsa_vec_get(v, 1, &p), CDSA_OK); CHECK_EQ(*(int *)p, 1);
    cdsa_vec_destroy(v);
}

static void test_erase_middle(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }
    CHECK_EQ(cdsa_vec_erase(v, 2, NULL), CDSA_OK);  /* remove element 2 */
    CHECK_EQ(cdsa_vec_len(v), (size_t)4);
    int expected[4] = {0, 1, 3, 4};
    void *p = NULL;
    for (int i = 0; i < 4; ++i) {
        CHECK_EQ(cdsa_vec_get(v, (size_t)i, &p), CDSA_OK);
        CHECK_EQ(*(int *)p, expected[i]);
    }
    cdsa_vec_destroy(v);
}

static void test_erase_single_element(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 99;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_erase(v, 0, NULL), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), (size_t)0);
    cdsa_vec_destroy(v);
}

static void test_erase_with_out(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), counting_free, &v), CDSA_OK);
    int vals[3] = {10, 20, 30};
    for (int i = 0; i < 3; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &vals[i]), CDSA_OK);
    }
    g_free_count = 0;
    int out = 0;
    CHECK_EQ(cdsa_vec_erase(v, 1, &out), CDSA_OK);
    CHECK_EQ(out, 20);
    CHECK_EQ(g_free_count, (size_t)0);  /* free_fn must NOT be called */
    CHECK_EQ(cdsa_vec_len(v), (size_t)2);
    cdsa_vec_destroy(v);
}

static void test_erase_null_out_calls_free_fn(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), counting_free, &v), CDSA_OK);
    int val = 7;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    g_free_count = 0;
    CHECK_EQ(cdsa_vec_erase(v, 0, NULL), CDSA_OK);
    CHECK_EQ(g_free_count, (size_t)1);
    cdsa_vec_destroy(v);
}

static void test_erase_null_vec(void)
{
    CHECK_EQ(cdsa_vec_erase(NULL, 0, NULL), CDSA_ERR_INVAL);
}

static void test_erase_empty(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    CHECK_EQ(cdsa_vec_erase(v, 0, NULL), CDSA_ERR_RANGE);
    cdsa_vec_destroy(v);
}

static void test_erase_out_of_range(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 1;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_erase(v, 1, NULL), CDSA_ERR_RANGE);
    cdsa_vec_destroy(v);
}

/* ── cdsa_vec_reserve ────────────────────────────────────────────────────── */

static void test_reserve_basic(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    CHECK_EQ(cdsa_vec_reserve(v, 100), CDSA_OK);
    CHECK_GE(cdsa_vec_cap(v), (size_t)100);
    CHECK_EQ(cdsa_vec_len(v), (size_t)0);
    cdsa_vec_destroy(v);
}

static void test_reserve_no_op_when_sufficient(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    CHECK_EQ(cdsa_vec_reserve(v, 10), CDSA_OK);
    size_t cap_after = cdsa_vec_cap(v);
    CHECK_EQ(cdsa_vec_reserve(v, 5), CDSA_OK);  /* 5 <= current cap → no-op */
    CHECK_EQ(cdsa_vec_cap(v), cap_after);
    cdsa_vec_destroy(v);
}

static void test_reserve_zero(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    CHECK_EQ(cdsa_vec_reserve(v, 0), CDSA_OK);
    CHECK_EQ(cdsa_vec_cap(v), (size_t)0);  /* already sufficient */
    cdsa_vec_destroy(v);
}

static void test_reserve_len_unchanged(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 1;
    for (int i = 0; i < 3; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    }
    CHECK_EQ(cdsa_vec_reserve(v, 50), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), (size_t)3);
    cdsa_vec_destroy(v);
}

static void test_reserve_null_vec(void)
{
    CHECK_EQ(cdsa_vec_reserve(NULL, 10), CDSA_ERR_INVAL);
}

static void test_reserve_alloc_failure(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    cdsa_alloc_fail_after(1);
    CHECK_EQ(cdsa_vec_reserve(v, 10), CDSA_ERR_NOMEM);
    CHECK_EQ(cdsa_vec_cap(v), (size_t)0);
    CHECK_EQ(cdsa_vec_len(v), (size_t)0);
    reset();
    cdsa_vec_destroy(v);
}

/* ── cdsa_vec_shrink_to_fit ──────────────────────────────────────────────── */

static void test_shrink_to_fit_basic(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    /* Push 5 elements to trigger at least one growth (initial cap 4 → 8). */
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }
    CHECK_GT(cdsa_vec_cap(v), cdsa_vec_len(v));
    CHECK_EQ(cdsa_vec_shrink_to_fit(v), CDSA_OK);
    CHECK_EQ(cdsa_vec_cap(v), cdsa_vec_len(v));
    CHECK_EQ(cdsa_vec_cap(v), (size_t)5);
    cdsa_vec_destroy(v);
}

static void test_shrink_to_fit_already_minimal(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    int val = 1;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    /* Reserve exactly one so cap == len == 1 after shrink. */
    CHECK_EQ(cdsa_vec_reserve(v, 1), CDSA_OK);
    CHECK_EQ(cdsa_vec_shrink_to_fit(v), CDSA_OK);  /* no-op */
    CHECK_EQ(cdsa_vec_cap(v), (size_t)1);
    cdsa_vec_destroy(v);
}

static void test_shrink_to_fit_to_zero(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    CHECK_EQ(cdsa_vec_reserve(v, 8), CDSA_OK);
    CHECK_EQ(cdsa_vec_cap(v), (size_t)8);
    CHECK_EQ(cdsa_vec_len(v), (size_t)0);
    CHECK_EQ(cdsa_vec_shrink_to_fit(v), CDSA_OK);
    CHECK_EQ(cdsa_vec_cap(v), (size_t)0);
    /* Push should still work after freeing the buffer. */
    int val = 99;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), (size_t)1);
    cdsa_vec_destroy(v);
}

static void test_shrink_to_fit_null_vec(void)
{
    CHECK_EQ(cdsa_vec_shrink_to_fit(NULL), CDSA_ERR_INVAL);
}

static void test_shrink_to_fit_alloc_failure(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }
    size_t cap_before = cdsa_vec_cap(v);
    size_t len_before = cdsa_vec_len(v);
    CHECK_GT(cap_before, len_before);

    cdsa_alloc_fail_after(1);
    CHECK_EQ(cdsa_vec_shrink_to_fit(v), CDSA_ERR_NOMEM);
    CHECK_EQ(cdsa_vec_cap(v), cap_before);
    CHECK_EQ(cdsa_vec_len(v), len_before);
    reset();

    cdsa_vec_destroy(v);
}

/* ── cdsa_vec_clear ──────────────────────────────────────────────────────── */

static void test_clear_empty(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    cdsa_vec_clear(v);  /* must not crash */
    CHECK_EQ(cdsa_vec_len(v), (size_t)0);
    cdsa_vec_destroy(v);
}

static void test_clear_basic(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }
    size_t cap_before = cdsa_vec_cap(v);
    cdsa_vec_clear(v);
    CHECK_EQ(cdsa_vec_len(v), (size_t)0);
    CHECK_EQ(cdsa_vec_cap(v), cap_before);  /* buffer kept */
    cdsa_vec_destroy(v);
}

static void test_clear_calls_free_fn(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), counting_free, &v), CDSA_OK);
    for (int i = 0; i < 4; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }
    g_free_count = 0;
    cdsa_vec_clear(v);
    CHECK_EQ(g_free_count, (size_t)4);
    CHECK_EQ(cdsa_vec_len(v), (size_t)0);
    cdsa_vec_destroy(v);
}

static void test_clear_null_vec(void)
{
    cdsa_vec_clear(NULL);  /* must not crash */
}

static void test_clear_then_push(void)
{
    cdsa_vec_t *v = NULL;
    CHECK_EQ(cdsa_vec_create(sizeof(int), NULL, &v), CDSA_OK);
    for (int i = 0; i < 3; ++i) {
        CHECK_EQ(cdsa_vec_push(v, &i), CDSA_OK);
    }
    cdsa_vec_clear(v);
    int val = 42;
    CHECK_EQ(cdsa_vec_push(v, &val), CDSA_OK);
    CHECK_EQ(cdsa_vec_len(v), (size_t)1);
    void *p = NULL;
    CHECK_EQ(cdsa_vec_get(v, 0, &p), CDSA_OK);
    CHECK_EQ(*(int *)p, 42);
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
    /* insert */
    CDSA_T(test_insert_at_end),
    CDSA_T(test_insert_at_front),
    CDSA_T(test_insert_at_middle),
    CDSA_T(test_insert_forces_grow),
    CDSA_T(test_insert_into_empty),
    CDSA_T(test_insert_null_vec),
    CDSA_T(test_insert_null_elem),
    CDSA_T(test_insert_out_of_range),
    CDSA_T(test_insert_alloc_failure),
    /* erase */
    CDSA_T(test_erase_front),
    CDSA_T(test_erase_back),
    CDSA_T(test_erase_middle),
    CDSA_T(test_erase_single_element),
    CDSA_T(test_erase_with_out),
    CDSA_T(test_erase_null_out_calls_free_fn),
    CDSA_T(test_erase_null_vec),
    CDSA_T(test_erase_empty),
    CDSA_T(test_erase_out_of_range),
    /* reserve */
    CDSA_T(test_reserve_basic),
    CDSA_T(test_reserve_no_op_when_sufficient),
    CDSA_T(test_reserve_zero),
    CDSA_T(test_reserve_len_unchanged),
    CDSA_T(test_reserve_null_vec),
    CDSA_T(test_reserve_alloc_failure),
    /* shrink_to_fit */
    CDSA_T(test_shrink_to_fit_basic),
    CDSA_T(test_shrink_to_fit_already_minimal),
    CDSA_T(test_shrink_to_fit_to_zero),
    CDSA_T(test_shrink_to_fit_null_vec),
    CDSA_T(test_shrink_to_fit_alloc_failure),
    /* clear */
    CDSA_T(test_clear_empty),
    CDSA_T(test_clear_basic),
    CDSA_T(test_clear_calls_free_fn),
    CDSA_T(test_clear_null_vec),
    CDSA_T(test_clear_then_push),
};

int main(void)
{
    return cdsa_run_suite("cdsa_vec", tests, CDSA_ARRAY_LEN(tests));
}
