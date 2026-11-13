#include "check.h"
#include "cdsa/cdsa_slist.h"
#include "cdsa/cdsa_alloc.h"
#include "cdsa/cdsa_callbacks.h"
#include "cdsa/cdsa_status.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ── Helpers ─────────────────────────────────────────────────────────────── */

static size_t g_free_count = 0;

static void counting_free(void *elem)
{
    (void)elem;
    ++g_free_count;
}

static void reset(void)
{
    cdsa_alloc_fail_after(0);
    cdsa_allocator_set(NULL);
    g_free_count = 0;
}

/* Collect list contents front-to-back into an int array; return count. */
static size_t collect(cdsa_slist_t *list, int *buf, size_t buf_len)
{
    size_t count = 0;
    cdsa_slist_node_t *node = NULL;
    CDSA_SLIST_FOREACH(list, node) {
        if (count < buf_len) {
            buf[count] = *(int *)cdsa_slist_node_data(node);
        }
        ++count;
    }
    return count;
}

/* ── cdsa_slist_create ───────────────────────────────────────────────────── */

static void test_create_basic(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    CHECK_NOTNULL(list);
    CHECK_EQ(cdsa_slist_len(list), (size_t)0);
    CHECK_NULL(cdsa_slist_head(list));
    cdsa_slist_destroy(list);
}

static void test_create_null_out(void)
{
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, NULL),
             CDSA_ERR_INVAL);
}

static void test_create_zero_elem_size(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(0, NULL, NULL, &list), CDSA_ERR_INVAL);
    CHECK_NULL(list);
}

static void test_create_alloc_failure_struct(void)
{
    cdsa_slist_t *list = NULL;
    cdsa_alloc_fail_after(1);
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list),
             CDSA_ERR_NOMEM);
    CHECK_NULL(list);
    reset();
}

static void test_create_alloc_failure_sentinel(void)
{
    cdsa_slist_t *list = NULL;
    cdsa_alloc_fail_after(2);  /* struct OK, sentinel fails */
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list),
             CDSA_ERR_NOMEM);
    CHECK_NULL(list);
    reset();
}

/* ── cdsa_slist_destroy ──────────────────────────────────────────────────── */

static void test_destroy_null(void)
{
    cdsa_slist_destroy(NULL); /* must not crash */
}

static void test_destroy_calls_free_fn(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, counting_free, &list),
             CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    v = 2;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    v = 3;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    g_free_count = 0;
    cdsa_slist_destroy(list);
    CHECK_EQ(g_free_count, (size_t)3);
}

static void test_destroy_empty(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, counting_free, &list),
             CDSA_OK);
    g_free_count = 0;
    cdsa_slist_destroy(list); /* must not crash, no free calls */
    CHECK_EQ(g_free_count, (size_t)0);
}

/* ── cdsa_slist_push_front ───────────────────────────────────────────────── */

static void test_push_front_basic(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);

    for (int i = 1; i <= 4; ++i) {
        CHECK_EQ(cdsa_slist_push_front(list, &i), CDSA_OK);
    }
    CHECK_EQ(cdsa_slist_len(list), (size_t)4);

    /* Elements should be in reverse push order: 4 3 2 1 */
    int buf[4] = {0};
    CHECK_EQ(collect(list, buf, 4), (size_t)4);
    CHECK_EQ(buf[0], 4);
    CHECK_EQ(buf[1], 3);
    CHECK_EQ(buf[2], 2);
    CHECK_EQ(buf[3], 1);
    cdsa_slist_destroy(list);
}

static void test_push_front_null_list(void)
{
    int v = 1;
    CHECK_EQ(cdsa_slist_push_front(NULL, &v), CDSA_ERR_INVAL);
}

static void test_push_front_null_elem(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    CHECK_EQ(cdsa_slist_push_front(list, NULL), CDSA_ERR_INVAL);
    cdsa_slist_destroy(list);
}

static void test_push_front_alloc_failure(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    cdsa_alloc_fail_after(1);
    int v = 42;
    CHECK_EQ(cdsa_slist_push_front(list, &v), CDSA_ERR_NOMEM);
    CHECK_EQ(cdsa_slist_len(list), (size_t)0);
    reset();
    cdsa_slist_destroy(list);
}

/* ── cdsa_slist_pop_front ────────────────────────────────────────────────── */

static void test_pop_front_basic(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 10;
    CHECK_EQ(cdsa_slist_push_front(list, &v), CDSA_OK);
    v = 20;
    CHECK_EQ(cdsa_slist_push_front(list, &v), CDSA_OK);

    int out = 0;
    CHECK_EQ(cdsa_slist_pop_front(list, &out), CDSA_OK);
    CHECK_EQ(out, 20);
    CHECK_EQ(cdsa_slist_len(list), (size_t)1);
    CHECK_EQ(cdsa_slist_pop_front(list, &out), CDSA_OK);
    CHECK_EQ(out, 10);
    CHECK_EQ(cdsa_slist_len(list), (size_t)0);
    cdsa_slist_destroy(list);
}

static void test_pop_front_empty(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    CHECK_EQ(cdsa_slist_pop_front(list, NULL), CDSA_ERR_EMPTY);
    cdsa_slist_destroy(list);
}

static void test_pop_front_null_list(void)
{
    CHECK_EQ(cdsa_slist_pop_front(NULL, NULL), CDSA_ERR_INVAL);
}

static void test_pop_front_calls_free_fn(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, counting_free, &list),
             CDSA_OK);
    int v = 5;
    CHECK_EQ(cdsa_slist_push_front(list, &v), CDSA_OK);
    g_free_count = 0;
    CHECK_EQ(cdsa_slist_pop_front(list, NULL), CDSA_OK); /* out==NULL→free */
    CHECK_EQ(g_free_count, (size_t)1);
    cdsa_slist_destroy(list);
}

static void test_pop_front_no_free_fn_with_out(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, counting_free, &list),
             CDSA_OK);
    int v = 7;
    CHECK_EQ(cdsa_slist_push_front(list, &v), CDSA_OK);
    g_free_count = 0;
    int out = 0;
    CHECK_EQ(cdsa_slist_pop_front(list, &out), CDSA_OK); /* out!=NULL→no free */
    CHECK_EQ(g_free_count, (size_t)0);
    CHECK_EQ(out, 7);
    cdsa_slist_destroy(list);
}

/* Popping the last element must reset tail to sentinel. */
static void test_pop_front_resets_tail(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 99;
    CHECK_EQ(cdsa_slist_push_front(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_slist_pop_front(list, NULL), CDSA_OK);
    CHECK_EQ(cdsa_slist_len(list), (size_t)0);
    CHECK_NULL(cdsa_slist_head(list));
    /* push_back after empty must still work */
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_slist_len(list), (size_t)1);
    cdsa_slist_destroy(list);
}

/* ── cdsa_slist_push_back ────────────────────────────────────────────────── */

static void test_push_back_basic(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);

    for (int i = 1; i <= 4; ++i) {
        CHECK_EQ(cdsa_slist_push_back(list, &i), CDSA_OK);
    }
    CHECK_EQ(cdsa_slist_len(list), (size_t)4);

    int buf[4] = {0};
    CHECK_EQ(collect(list, buf, 4), (size_t)4);
    for (int i = 0; i < 4; ++i) {
        CHECK_EQ(buf[i], i + 1);
    }
    cdsa_slist_destroy(list);
}

static void test_push_back_null_list(void)
{
    int v = 1;
    CHECK_EQ(cdsa_slist_push_back(NULL, &v), CDSA_ERR_INVAL);
}

static void test_push_back_null_elem(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    CHECK_EQ(cdsa_slist_push_back(list, NULL), CDSA_ERR_INVAL);
    cdsa_slist_destroy(list);
}

static void test_push_back_alloc_failure(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    cdsa_alloc_fail_after(1);
    int v = 42;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_ERR_NOMEM);
    CHECK_EQ(cdsa_slist_len(list), (size_t)0);
    reset();
    cdsa_slist_destroy(list);
}

/* ── cdsa_slist_pop_back ─────────────────────────────────────────────────── */

static void test_pop_back_basic(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 3; ++i) {
        CHECK_EQ(cdsa_slist_push_back(list, &i), CDSA_OK);
    }
    int out = 0;
    CHECK_EQ(cdsa_slist_pop_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 3);
    CHECK_EQ(cdsa_slist_len(list), (size_t)2);
    CHECK_EQ(cdsa_slist_pop_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 2);
    cdsa_slist_destroy(list);
}

static void test_pop_back_empty(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    CHECK_EQ(cdsa_slist_pop_back(list, NULL), CDSA_ERR_EMPTY);
    cdsa_slist_destroy(list);
}

static void test_pop_back_null_list(void)
{
    CHECK_EQ(cdsa_slist_pop_back(NULL, NULL), CDSA_ERR_INVAL);
}

static void test_pop_back_single_element(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 42;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    int out = 0;
    CHECK_EQ(cdsa_slist_pop_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 42);
    CHECK_EQ(cdsa_slist_len(list), (size_t)0);
    CHECK_NULL(cdsa_slist_head(list));
    /* Should still work after empty */
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_slist_len(list), (size_t)1);
    cdsa_slist_destroy(list);
}

static void test_pop_back_calls_free_fn(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, counting_free, &list),
             CDSA_OK);
    int v = 5;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    g_free_count = 0;
    CHECK_EQ(cdsa_slist_pop_back(list, NULL), CDSA_OK);
    CHECK_EQ(g_free_count, (size_t)1);
    cdsa_slist_destroy(list);
}

/* ── Peek ────────────────────────────────────────────────────────────────── */

static void test_peek_front_basic(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 10;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    v = 20;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    int out = 0;
    CHECK_EQ(cdsa_slist_peek_front(list, &out), CDSA_OK);
    CHECK_EQ(out, 10);
    CHECK_EQ(cdsa_slist_len(list), (size_t)2); /* unchanged */
    cdsa_slist_destroy(list);
}

static void test_peek_back_basic(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 10;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    v = 20;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    int out = 0;
    CHECK_EQ(cdsa_slist_peek_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 20);
    CHECK_EQ(cdsa_slist_len(list), (size_t)2); /* unchanged */
    cdsa_slist_destroy(list);
}

static void test_peek_front_empty(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int out = 0;
    CHECK_EQ(cdsa_slist_peek_front(list, &out), CDSA_ERR_EMPTY);
    cdsa_slist_destroy(list);
}

static void test_peek_back_empty(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int out = 0;
    CHECK_EQ(cdsa_slist_peek_back(list, &out), CDSA_ERR_EMPTY);
    cdsa_slist_destroy(list);
}

static void test_peek_null_out(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_slist_peek_front(list, NULL), CDSA_ERR_INVAL);
    CHECK_EQ(cdsa_slist_peek_back(list, NULL), CDSA_ERR_INVAL);
    cdsa_slist_destroy(list);
}

/* ── cdsa_slist_find ─────────────────────────────────────────────────────── */

static void test_find_basic(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    for (int i = 1; i <= 5; ++i) {
        CHECK_EQ(cdsa_slist_push_back(list, &i), CDSA_OK);
    }
    int key = 3;
    cdsa_slist_node_t *node = NULL;
    CHECK_EQ(cdsa_slist_find(list, &key, &node), CDSA_OK);
    CHECK_NOTNULL(node);
    CHECK_EQ(*(int *)cdsa_slist_node_data(node), 3);
    cdsa_slist_destroy(list);
}

static void test_find_first_element(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    int v = 42;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    cdsa_slist_node_t *node = NULL;
    CHECK_EQ(cdsa_slist_find(list, &v, &node), CDSA_OK);
    CHECK_EQ(*(int *)cdsa_slist_node_data(node), 42);
    cdsa_slist_destroy(list);
}

static void test_find_not_found(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    int key = 99;
    cdsa_slist_node_t *node = NULL;
    CHECK_EQ(cdsa_slist_find(list, &key, &node), CDSA_ERR_NOTFOUND);
    cdsa_slist_destroy(list);
}

static void test_find_empty(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    int key = 5;
    cdsa_slist_node_t *node = NULL;
    CHECK_EQ(cdsa_slist_find(list, &key, &node), CDSA_ERR_NOTFOUND);
    cdsa_slist_destroy(list);
}

static void test_find_no_cmp_fn(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    cdsa_slist_node_t *node = NULL;
    CHECK_EQ(cdsa_slist_find(list, &v, &node), CDSA_ERR_INVAL);
    cdsa_slist_destroy(list);
}

static void test_find_first_occurrence(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    int v = 7;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK); /* first 7 */
    int v2 = 7;
    CHECK_EQ(cdsa_slist_push_back(list, &v2), CDSA_OK); /* second 7 */
    cdsa_slist_node_t *found = NULL;
    CHECK_EQ(cdsa_slist_find(list, &v, &found), CDSA_OK);
    /* Must be the first node */
    CHECK_EQ(found, cdsa_slist_head(list));
    cdsa_slist_destroy(list);
}

/* ── cdsa_slist_remove ───────────────────────────────────────────────────── */

static void test_remove_basic(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    for (int i = 1; i <= 5; ++i) {
        CHECK_EQ(cdsa_slist_push_back(list, &i), CDSA_OK);
    }
    int key = 3;
    int out = 0;
    CHECK_EQ(cdsa_slist_remove(list, &key, &out), CDSA_OK);
    CHECK_EQ(out, 3);
    CHECK_EQ(cdsa_slist_len(list), (size_t)4);

    int buf[4] = {0};
    CHECK_EQ(collect(list, buf, 4), (size_t)4);
    CHECK_EQ(buf[0], 1);
    CHECK_EQ(buf[1], 2);
    CHECK_EQ(buf[2], 4);
    CHECK_EQ(buf[3], 5);
    cdsa_slist_destroy(list);
}

static void test_remove_front(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    for (int i = 1; i <= 3; ++i) {
        CHECK_EQ(cdsa_slist_push_back(list, &i), CDSA_OK);
    }
    int key = 1;
    CHECK_EQ(cdsa_slist_remove(list, &key, NULL), CDSA_OK);
    CHECK_EQ(cdsa_slist_len(list), (size_t)2);
    int out = 0;
    CHECK_EQ(cdsa_slist_peek_front(list, &out), CDSA_OK);
    CHECK_EQ(out, 2);
    cdsa_slist_destroy(list);
}

static void test_remove_back(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    for (int i = 1; i <= 3; ++i) {
        CHECK_EQ(cdsa_slist_push_back(list, &i), CDSA_OK);
    }
    int key = 3;
    CHECK_EQ(cdsa_slist_remove(list, &key, NULL), CDSA_OK);
    CHECK_EQ(cdsa_slist_len(list), (size_t)2);
    int out = 0;
    CHECK_EQ(cdsa_slist_peek_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 2);
    cdsa_slist_destroy(list);
}

static void test_remove_not_found(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    int key = 99;
    CHECK_EQ(cdsa_slist_remove(list, &key, NULL), CDSA_ERR_NOTFOUND);
    CHECK_EQ(cdsa_slist_len(list), (size_t)1);
    cdsa_slist_destroy(list);
}

static void test_remove_calls_free_fn(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, counting_free,
                                &list),
             CDSA_OK);
    int v = 5;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    g_free_count = 0;
    CHECK_EQ(cdsa_slist_remove(list, &v, NULL), CDSA_OK);
    CHECK_EQ(g_free_count, (size_t)1);
    cdsa_slist_destroy(list);
}

static void test_remove_no_free_fn_with_out(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, counting_free,
                                &list),
             CDSA_OK);
    int v = 5;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    g_free_count = 0;
    int out = 0;
    CHECK_EQ(cdsa_slist_remove(list, &v, &out), CDSA_OK); /* out!=NULL→no free */
    CHECK_EQ(g_free_count, (size_t)0);
    CHECK_EQ(out, 5);
    cdsa_slist_destroy(list);
}

static void test_remove_no_cmp_fn(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_slist_remove(list, &v, NULL), CDSA_ERR_INVAL);
    cdsa_slist_destroy(list);
}

static void test_remove_first_occurrence_only(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    int v = 7;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_slist_remove(list, &v, NULL), CDSA_OK);
    CHECK_EQ(cdsa_slist_len(list), (size_t)2); /* only one removed */
    cdsa_slist_destroy(list);
}

static void test_remove_until_empty(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_slist_remove(list, &v, NULL), CDSA_OK);
    CHECK_EQ(cdsa_slist_len(list), (size_t)0);
    CHECK_NULL(cdsa_slist_head(list));
    /* push still works after empty via remove */
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_slist_len(list), (size_t)1);
    cdsa_slist_destroy(list);
}

/* ── Accessors ───────────────────────────────────────────────────────────── */

static void test_len_null(void)
{
    CHECK_EQ(cdsa_slist_len(NULL), (size_t)0);
}

static void test_head_null(void)
{
    CHECK_NULL(cdsa_slist_head(NULL));
}

static void test_node_next_null(void)
{
    CHECK_NULL(cdsa_slist_node_next(NULL));
}

static void test_node_data_null(void)
{
    CHECK_NULL(cdsa_slist_node_data(NULL));
}

/* ── FIFO and LIFO behaviour ─────────────────────────────────────────────── */

static void test_fifo_via_push_back_pop_front(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 5; ++i) {
        CHECK_EQ(cdsa_slist_push_back(list, &i), CDSA_OK);
    }
    for (int expected = 1; expected <= 5; ++expected) {
        int out = 0;
        CHECK_EQ(cdsa_slist_pop_front(list, &out), CDSA_OK);
        CHECK_EQ(out, expected);
    }
    CHECK_EQ(cdsa_slist_pop_front(list, NULL), CDSA_ERR_EMPTY);
    cdsa_slist_destroy(list);
}

static void test_lifo_via_push_front_pop_front(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 5; ++i) {
        CHECK_EQ(cdsa_slist_push_front(list, &i), CDSA_OK);
    }
    for (int expected = 5; expected >= 1; --expected) {
        int out = 0;
        CHECK_EQ(cdsa_slist_pop_front(list, &out), CDSA_OK);
        CHECK_EQ(out, expected);
    }
    cdsa_slist_destroy(list);
}

static void test_mixed_push_front_back(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 2;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    v = 3;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
    v = 1;
    CHECK_EQ(cdsa_slist_push_front(list, &v), CDSA_OK);
    v = 4;
    CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);

    int buf[4] = {0};
    CHECK_EQ(collect(list, buf, 4), (size_t)4);
    CHECK_EQ(buf[0], 1);
    CHECK_EQ(buf[1], 2);
    CHECK_EQ(buf[2], 3);
    CHECK_EQ(buf[3], 4);
    cdsa_slist_destroy(list);
}

/* ── CDSA_SLIST_FOREACH ──────────────────────────────────────────────────── */

static void test_foreach_empty(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int iterations = 0;
    cdsa_slist_node_t *node = NULL;
    CDSA_SLIST_FOREACH(list, node) {
        ++iterations;
    }
    CHECK_EQ(iterations, 0);
    cdsa_slist_destroy(list);
}

static void test_foreach_in_order(void)
{
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(cdsa_slist_push_back(list, &i), CDSA_OK);
    }
    int expected = 0;
    cdsa_slist_node_t *node = NULL;
    CDSA_SLIST_FOREACH(list, node) {
        CHECK_EQ(*(int *)cdsa_slist_node_data(node), expected);
        ++expected;
    }
    CHECK_EQ(expected, 5);
    cdsa_slist_destroy(list);
}

/* ── Randomized differential test ─────────────────────────────────────────── */

#define RTEST_MAX  128
#define RTEST_OPS  2000

static uint32_t g_rng = 0u;

static uint32_t rng_next(void)
{
    g_rng = (uint32_t)(g_rng * UINT32_C(1664525) + UINT32_C(1013904223));
    return g_rng;
}

typedef struct { int data[RTEST_MAX]; size_t len; } rtest_ref_t;

static void ref_push_front(rtest_ref_t *r, int v)
{
    memmove(&r->data[1], &r->data[0], r->len * sizeof(int));
    r->data[0] = v;
    r->len++;
}

static void ref_pop_front(rtest_ref_t *r)
{
    memmove(&r->data[0], &r->data[1], (r->len - 1u) * sizeof(int));
    r->len--;
}

static void ref_push_back(rtest_ref_t *r, int v)
{
    r->data[r->len] = v;
    r->len++;
}

static void ref_pop_back(rtest_ref_t *r)
{
    r->len--;
}

static int ref_remove(rtest_ref_t *r, int key)
{
    for (size_t i = 0u; i < r->len; ++i) {
        if (r->data[i] == key) {
            memmove(&r->data[i], &r->data[i + 1u],
                    (r->len - i - 1u) * sizeof(int));
            r->len--;
            return 1;
        }
    }
    return 0;
}

static void rtest_check(cdsa_slist_t *list, const rtest_ref_t *r)
{
    if (cdsa_slist_len(list) != r->len) {
        cdsa_check_failures++;
        return;
    }
    size_t k = 0u;
    cdsa_slist_node_t *node = NULL;
    CDSA_SLIST_FOREACH(list, node) {
        CHECK_EQ(*(int *)cdsa_slist_node_data(node), r->data[k]);
        ++k;
    }
}

static void test_rand_diff(void)
{
    rtest_ref_t  ref  = {{0}, 0u};
    cdsa_slist_t *list = NULL;
    CHECK_EQ(cdsa_slist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    if (list == NULL) {
        return;
    }

    g_rng = 42u;

    for (int iter = 0; iter < RTEST_OPS; ++iter) {
        uint32_t rng = rng_next();
        int      v   = (int)(rng_next() % 32u); /* small range → more duplicates */

        switch (rng % 5u) {
        case 0u:
            if (ref.len < (size_t)RTEST_MAX) {
                CHECK_EQ(cdsa_slist_push_front(list, &v), CDSA_OK);
                ref_push_front(&ref, v);
            }
            break;
        case 1u:
            if (ref.len > 0u) {
                CHECK_EQ(cdsa_slist_pop_front(list, NULL), CDSA_OK);
                ref_pop_front(&ref);
            } else {
                CHECK_EQ(cdsa_slist_pop_front(list, NULL), CDSA_ERR_EMPTY);
            }
            break;
        case 2u:
            if (ref.len < (size_t)RTEST_MAX) {
                CHECK_EQ(cdsa_slist_push_back(list, &v), CDSA_OK);
                ref_push_back(&ref, v);
            }
            break;
        case 3u:
            if (ref.len > 0u) {
                CHECK_EQ(cdsa_slist_pop_back(list, NULL), CDSA_OK);
                ref_pop_back(&ref);
            } else {
                CHECK_EQ(cdsa_slist_pop_back(list, NULL), CDSA_ERR_EMPTY);
            }
            break;
        case 4u: {
            int found_ref = ref_remove(&ref, v);
            cdsa_status_t rc = cdsa_slist_remove(list, &v, NULL);
            CHECK_EQ(rc, found_ref ? CDSA_OK : CDSA_ERR_NOTFOUND);
            break;
        }
        default:
            break;
        }

        rtest_check(list, &ref);
    }

    cdsa_slist_destroy(list);
}

/* ── Test registry ───────────────────────────────────────────────────────── */

static const cdsa_test_entry_t tests[] = {
    /* create */
    CDSA_T(test_create_basic),
    CDSA_T(test_create_null_out),
    CDSA_T(test_create_zero_elem_size),
    CDSA_T(test_create_alloc_failure_struct),
    CDSA_T(test_create_alloc_failure_sentinel),
    /* destroy */
    CDSA_T(test_destroy_null),
    CDSA_T(test_destroy_calls_free_fn),
    CDSA_T(test_destroy_empty),
    /* push_front */
    CDSA_T(test_push_front_basic),
    CDSA_T(test_push_front_null_list),
    CDSA_T(test_push_front_null_elem),
    CDSA_T(test_push_front_alloc_failure),
    /* pop_front */
    CDSA_T(test_pop_front_basic),
    CDSA_T(test_pop_front_empty),
    CDSA_T(test_pop_front_null_list),
    CDSA_T(test_pop_front_calls_free_fn),
    CDSA_T(test_pop_front_no_free_fn_with_out),
    CDSA_T(test_pop_front_resets_tail),
    /* push_back */
    CDSA_T(test_push_back_basic),
    CDSA_T(test_push_back_null_list),
    CDSA_T(test_push_back_null_elem),
    CDSA_T(test_push_back_alloc_failure),
    /* pop_back */
    CDSA_T(test_pop_back_basic),
    CDSA_T(test_pop_back_empty),
    CDSA_T(test_pop_back_null_list),
    CDSA_T(test_pop_back_single_element),
    CDSA_T(test_pop_back_calls_free_fn),
    /* peek */
    CDSA_T(test_peek_front_basic),
    CDSA_T(test_peek_back_basic),
    CDSA_T(test_peek_front_empty),
    CDSA_T(test_peek_back_empty),
    CDSA_T(test_peek_null_out),
    /* find */
    CDSA_T(test_find_basic),
    CDSA_T(test_find_first_element),
    CDSA_T(test_find_not_found),
    CDSA_T(test_find_empty),
    CDSA_T(test_find_no_cmp_fn),
    CDSA_T(test_find_first_occurrence),
    /* remove */
    CDSA_T(test_remove_basic),
    CDSA_T(test_remove_front),
    CDSA_T(test_remove_back),
    CDSA_T(test_remove_not_found),
    CDSA_T(test_remove_calls_free_fn),
    CDSA_T(test_remove_no_free_fn_with_out),
    CDSA_T(test_remove_no_cmp_fn),
    CDSA_T(test_remove_first_occurrence_only),
    CDSA_T(test_remove_until_empty),
    /* accessors */
    CDSA_T(test_len_null),
    CDSA_T(test_head_null),
    CDSA_T(test_node_next_null),
    CDSA_T(test_node_data_null),
    /* fifo/lifo */
    CDSA_T(test_fifo_via_push_back_pop_front),
    CDSA_T(test_lifo_via_push_front_pop_front),
    CDSA_T(test_mixed_push_front_back),
    /* foreach */
    CDSA_T(test_foreach_empty),
    CDSA_T(test_foreach_in_order),
    /* randomized */
    CDSA_T(test_rand_diff),
};

int main(void)
{
    return cdsa_run_suite("cdsa_slist", tests, CDSA_ARRAY_LEN(tests));
}
