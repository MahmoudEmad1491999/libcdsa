#include "check.h"
#include "cdsa/cdsa_dlist.h"
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
static size_t collect(cdsa_dlist_t *list, int *buf, size_t buf_len)
{
    size_t count = 0;
    cdsa_dlist_node_t *node = NULL;
    CDSA_DLIST_FOREACH(list, node) {
        if (count < buf_len) {
            buf[count] = *(int *)cdsa_dlist_node_data(node);
        }
        ++count;
    }
    return count;
}

/* Collect list contents back-to-front. */
static size_t collect_rev(cdsa_dlist_t *list, int *buf, size_t buf_len)
{
    size_t count = 0;
    cdsa_dlist_node_t *node = NULL;
    CDSA_DLIST_FOREACH_REVERSE(list, node) {
        if (count < buf_len) {
            buf[count] = *(int *)cdsa_dlist_node_data(node);
        }
        ++count;
    }
    return count;
}

/* ── cdsa_dlist_create ───────────────────────────────────────────────────── */

static void test_create_basic(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    CHECK_NOTNULL(list);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)0);
    CHECK_NULL(cdsa_dlist_head(list));
    CHECK_NULL(cdsa_dlist_tail(list));
    cdsa_dlist_destroy(list);
}

static void test_create_null_out(void)
{
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, NULL),
             CDSA_ERR_INVAL);
}

static void test_create_zero_elem_size(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(0, NULL, NULL, &list), CDSA_ERR_INVAL);
    CHECK_NULL(list);
}

static void test_create_alloc_failure_struct(void)
{
    cdsa_dlist_t *list = NULL;
    cdsa_alloc_fail_after(1);
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list),
             CDSA_ERR_NOMEM);
    CHECK_NULL(list);
    reset();
}

static void test_create_alloc_failure_sentinel(void)
{
    cdsa_dlist_t *list = NULL;
    cdsa_alloc_fail_after(2); /* struct OK, sentinel fails */
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list),
             CDSA_ERR_NOMEM);
    CHECK_NULL(list);
    reset();
}

/* ── cdsa_dlist_destroy ──────────────────────────────────────────────────── */

static void test_destroy_null(void)
{
    cdsa_dlist_destroy(NULL); /* must not crash */
}

static void test_destroy_calls_free_fn(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, counting_free, &list),
             CDSA_OK);
    for (int i = 1; i <= 3; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }
    g_free_count = 0;
    cdsa_dlist_destroy(list);
    CHECK_EQ(g_free_count, (size_t)3);
}

/* ── cdsa_dlist_push_front / pop_front ───────────────────────────────────── */

static void test_push_front_basic(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 4; ++i) {
        CHECK_EQ(cdsa_dlist_push_front(list, &i), CDSA_OK);
    }
    CHECK_EQ(cdsa_dlist_len(list), (size_t)4);
    int buf[4] = {0};
    CHECK_EQ(collect(list, buf, 4), (size_t)4);
    CHECK_EQ(buf[0], 4);
    CHECK_EQ(buf[1], 3);
    CHECK_EQ(buf[2], 2);
    CHECK_EQ(buf[3], 1);
    cdsa_dlist_destroy(list);
}

static void test_pop_front_basic(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 10;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    v = 20;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    int out = 0;
    CHECK_EQ(cdsa_dlist_pop_front(list, &out), CDSA_OK);
    CHECK_EQ(out, 10);
    CHECK_EQ(cdsa_dlist_pop_front(list, &out), CDSA_OK);
    CHECK_EQ(out, 20);
    CHECK_EQ(cdsa_dlist_pop_front(list, NULL), CDSA_ERR_EMPTY);
    cdsa_dlist_destroy(list);
}

static void test_pop_front_null_list(void)
{
    CHECK_EQ(cdsa_dlist_pop_front(NULL, NULL), CDSA_ERR_INVAL);
}

static void test_pop_front_alloc_failure(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    cdsa_alloc_fail_after(1);
    int v = 42;
    CHECK_EQ(cdsa_dlist_push_front(list, &v), CDSA_ERR_NOMEM);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)0);
    reset();
    cdsa_dlist_destroy(list);
}

/* ── cdsa_dlist_push_back / pop_back ─────────────────────────────────────── */

static void test_push_pop_back_basic(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 4; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }
    CHECK_EQ(cdsa_dlist_len(list), (size_t)4);

    int buf[4] = {0};
    CHECK_EQ(collect(list, buf, 4), (size_t)4);
    for (int i = 0; i < 4; ++i) {
        CHECK_EQ(buf[i], i + 1);
    }

    int out = 0;
    CHECK_EQ(cdsa_dlist_pop_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 4);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)3);
    CHECK_EQ(cdsa_dlist_pop_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 3);
    cdsa_dlist_destroy(list);
}

static void test_pop_back_single_element(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 99;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    CHECK_EQ(cdsa_dlist_pop_back(list, NULL), CDSA_OK);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)0);
    CHECK_NULL(cdsa_dlist_head(list));
    CHECK_NULL(cdsa_dlist_tail(list));
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK); /* still works after empty */
    cdsa_dlist_destroy(list);
}

static void test_pop_back_calls_free_fn(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, counting_free, &list),
             CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    g_free_count = 0;
    CHECK_EQ(cdsa_dlist_pop_back(list, NULL), CDSA_OK);
    CHECK_EQ(g_free_count, (size_t)1);
    cdsa_dlist_destroy(list);
}

/* ── cdsa_dlist_peek ─────────────────────────────────────────────────────── */

static void test_peek_front_back(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 10;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    v = 20;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    int out = 0;
    CHECK_EQ(cdsa_dlist_peek_front(list, &out), CDSA_OK);
    CHECK_EQ(out, 10);
    CHECK_EQ(cdsa_dlist_peek_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 20);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)2); /* unchanged */
    cdsa_dlist_destroy(list);
}

static void test_peek_empty(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int out = 0;
    CHECK_EQ(cdsa_dlist_peek_front(list, &out), CDSA_ERR_EMPTY);
    CHECK_EQ(cdsa_dlist_peek_back(list, &out), CDSA_ERR_EMPTY);
    cdsa_dlist_destroy(list);
}

/* ── cdsa_dlist_insert_before ────────────────────────────────────────────── */

static void test_insert_before_middle(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK); /* [1] */
    v = 3;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK); /* [1,3] */

    cdsa_dlist_node_t *n3 = cdsa_dlist_tail(list);
    v = 2;
    CHECK_EQ(cdsa_dlist_insert_before(list, n3, &v), CDSA_OK); /* [1,2,3] */
    CHECK_EQ(cdsa_dlist_len(list), (size_t)3);

    int buf[3] = {0};
    CHECK_EQ(collect(list, buf, 3), (size_t)3);
    CHECK_EQ(buf[0], 1);
    CHECK_EQ(buf[1], 2);
    CHECK_EQ(buf[2], 3);
    cdsa_dlist_destroy(list);
}

static void test_insert_before_head(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 2;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK); /* [2] */

    cdsa_dlist_node_t *head = cdsa_dlist_head(list);
    v = 1;
    CHECK_EQ(cdsa_dlist_insert_before(list, head, &v), CDSA_OK); /* [1,2] */
    CHECK_EQ(cdsa_dlist_len(list), (size_t)2);

    int buf[2] = {0};
    CHECK_EQ(collect(list, buf, 2), (size_t)2);
    CHECK_EQ(buf[0], 1);
    CHECK_EQ(buf[1], 2);
    cdsa_dlist_destroy(list);
}

static void test_insert_before_null_args(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    cdsa_dlist_node_t *node = cdsa_dlist_head(list);
    CHECK_EQ(cdsa_dlist_insert_before(NULL, node, &v), CDSA_ERR_INVAL);
    CHECK_EQ(cdsa_dlist_insert_before(list, NULL, &v), CDSA_ERR_INVAL);
    CHECK_EQ(cdsa_dlist_insert_before(list, node, NULL), CDSA_ERR_INVAL);
    cdsa_dlist_destroy(list);
}

static void test_insert_before_alloc_failure(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    cdsa_dlist_node_t *node = cdsa_dlist_head(list);
    cdsa_alloc_fail_after(1);
    v = 0;
    CHECK_EQ(cdsa_dlist_insert_before(list, node, &v), CDSA_ERR_NOMEM);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)1);
    reset();
    cdsa_dlist_destroy(list);
}

/* ── cdsa_dlist_insert_after ─────────────────────────────────────────────── */

static void test_insert_after_middle(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK); /* [1] */
    v = 3;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK); /* [1,3] */

    cdsa_dlist_node_t *n1 = cdsa_dlist_head(list);
    v = 2;
    CHECK_EQ(cdsa_dlist_insert_after(list, n1, &v), CDSA_OK); /* [1,2,3] */
    CHECK_EQ(cdsa_dlist_len(list), (size_t)3);

    int buf[3] = {0};
    CHECK_EQ(collect(list, buf, 3), (size_t)3);
    CHECK_EQ(buf[0], 1);
    CHECK_EQ(buf[1], 2);
    CHECK_EQ(buf[2], 3);
    cdsa_dlist_destroy(list);
}

static void test_insert_after_tail(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK); /* [1] */

    cdsa_dlist_node_t *tail = cdsa_dlist_tail(list);
    v = 2;
    CHECK_EQ(cdsa_dlist_insert_after(list, tail, &v), CDSA_OK); /* [1,2] */

    /* Tail pointer must have been updated. */
    int out = 0;
    CHECK_EQ(cdsa_dlist_peek_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 2);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)2);
    cdsa_dlist_destroy(list);
}

static void test_insert_after_null_args(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    cdsa_dlist_node_t *node = cdsa_dlist_head(list);
    CHECK_EQ(cdsa_dlist_insert_after(NULL, node, &v), CDSA_ERR_INVAL);
    CHECK_EQ(cdsa_dlist_insert_after(list, NULL, &v), CDSA_ERR_INVAL);
    CHECK_EQ(cdsa_dlist_insert_after(list, node, NULL), CDSA_ERR_INVAL);
    cdsa_dlist_destroy(list);
}

/* Insert before/after consistency: both express the same relative ordering. */
static void test_insert_before_after_symmetry(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);

    /* Build [10,20,30,40,50] using alternating insert_before and insert_after. */
    int v = 30;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);       /* [30]            */
    cdsa_dlist_node_t *n30 = cdsa_dlist_head(list);
    v = 20;
    CHECK_EQ(cdsa_dlist_insert_before(list, n30, &v), CDSA_OK); /* [20,30]      */
    cdsa_dlist_node_t *n20 = cdsa_dlist_head(list);
    v = 40;
    CHECK_EQ(cdsa_dlist_insert_after(list, n30, &v), CDSA_OK);  /* [20,30,40]   */
    cdsa_dlist_node_t *n40 = cdsa_dlist_tail(list);
    v = 50;
    CHECK_EQ(cdsa_dlist_insert_after(list, n40, &v), CDSA_OK);  /* [20,30,40,50] */
    v = 10;
    CHECK_EQ(cdsa_dlist_insert_before(list, n20, &v), CDSA_OK); /* [10,20,30,40,50] */

    int buf[5] = {0};
    CHECK_EQ(collect(list, buf, 5), (size_t)5);
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(buf[i], (i + 1) * 10);
    }
    cdsa_dlist_destroy(list);
}

/* ── cdsa_dlist_remove_node ──────────────────────────────────────────────── */

static void test_remove_node_middle(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 5; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }
    /* Remove the node with value 3 (3rd position). */
    cdsa_dlist_node_t *cur = cdsa_dlist_head(list);
    cur = cdsa_dlist_node_next(cur); /* skip 1 */
    cur = cdsa_dlist_node_next(cur); /* skip 2 */
    int out = 0;
    CHECK_EQ(cdsa_dlist_remove_node(list, cur, &out), CDSA_OK);
    CHECK_EQ(out, 3);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)4);

    int buf[4] = {0};
    CHECK_EQ(collect(list, buf, 4), (size_t)4);
    CHECK_EQ(buf[0], 1);
    CHECK_EQ(buf[1], 2);
    CHECK_EQ(buf[2], 4);
    CHECK_EQ(buf[3], 5);
    cdsa_dlist_destroy(list);
}

static void test_remove_node_head(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 3; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }
    CHECK_EQ(cdsa_dlist_remove_node(list, cdsa_dlist_head(list), NULL), CDSA_OK);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)2);
    int out = 0;
    CHECK_EQ(cdsa_dlist_peek_front(list, &out), CDSA_OK);
    CHECK_EQ(out, 2);
    cdsa_dlist_destroy(list);
}

static void test_remove_node_tail(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 3; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }
    CHECK_EQ(cdsa_dlist_remove_node(list, cdsa_dlist_tail(list), NULL), CDSA_OK);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)2);
    int out = 0;
    CHECK_EQ(cdsa_dlist_peek_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 2);
    cdsa_dlist_destroy(list);
}

static void test_remove_node_null_args(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    cdsa_dlist_node_t *node = cdsa_dlist_head(list);
    CHECK_EQ(cdsa_dlist_remove_node(NULL, node, NULL), CDSA_ERR_INVAL);
    CHECK_EQ(cdsa_dlist_remove_node(list, NULL, NULL), CDSA_ERR_INVAL);
    cdsa_dlist_destroy(list);
}

static void test_remove_node_calls_free_fn(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, counting_free, &list),
             CDSA_OK);
    int v = 7;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    g_free_count = 0;
    CHECK_EQ(cdsa_dlist_remove_node(list, cdsa_dlist_head(list), NULL), CDSA_OK);
    CHECK_EQ(g_free_count, (size_t)1);
    cdsa_dlist_destroy(list);
}

/* ── cdsa_dlist_find / remove ────────────────────────────────────────────── */

static void test_find_basic(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    for (int i = 1; i <= 5; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }
    int key = 3;
    cdsa_dlist_node_t *node = NULL;
    CHECK_EQ(cdsa_dlist_find(list, &key, &node), CDSA_OK);
    CHECK_NOTNULL(node);
    CHECK_EQ(*(int *)cdsa_dlist_node_data(node), 3);
    cdsa_dlist_destroy(list);
}

static void test_find_not_found(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    int key = 99;
    cdsa_dlist_node_t *node = NULL;
    CHECK_EQ(cdsa_dlist_find(list, &key, &node), CDSA_ERR_NOTFOUND);
    cdsa_dlist_destroy(list);
}

static void test_remove_basic(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    for (int i = 1; i <= 4; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }
    int key = 2;
    int out = 0;
    CHECK_EQ(cdsa_dlist_remove(list, &key, &out), CDSA_OK);
    CHECK_EQ(out, 2);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)3);

    int buf[3] = {0};
    CHECK_EQ(collect(list, buf, 3), (size_t)3);
    CHECK_EQ(buf[0], 1);
    CHECK_EQ(buf[1], 3);
    CHECK_EQ(buf[2], 4);
    cdsa_dlist_destroy(list);
}

/* ── Bidirectional traversal ─────────────────────────────────────────────── */

static void test_node_prev_basic(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 4; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }

    /* Forward: 1 2 3 4; Reverse: 4 3 2 1 */
    int buf[4] = {0};
    CHECK_EQ(collect_rev(list, buf, 4), (size_t)4);
    CHECK_EQ(buf[0], 4);
    CHECK_EQ(buf[1], 3);
    CHECK_EQ(buf[2], 2);
    CHECK_EQ(buf[3], 1);

    /* node_prev on the head must return NULL. */
    CHECK_NULL(cdsa_dlist_node_prev(cdsa_dlist_head(list)));
    cdsa_dlist_destroy(list);
}

static void test_node_prev_null(void)
{
    CHECK_NULL(cdsa_dlist_node_prev(NULL));
}

static void test_node_next_null(void)
{
    CHECK_NULL(cdsa_dlist_node_next(NULL));
}

/* ── cdsa_dlist_splice ───────────────────────────────────────────────────── */

static void test_splice_append(void)
{
    cdsa_dlist_t *dest = NULL;
    cdsa_dlist_t *src  = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &dest), CDSA_OK);
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &src),  CDSA_OK);

    for (int i = 1; i <= 3; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(dest, &i), CDSA_OK);
    }
    for (int i = 4; i <= 6; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(src, &i), CDSA_OK);
    }

    /* Splice src to end of dest (pos == NULL). */
    CHECK_EQ(cdsa_dlist_splice(dest, NULL, src), CDSA_OK);
    CHECK_EQ(cdsa_dlist_len(dest), (size_t)6);
    CHECK_EQ(cdsa_dlist_len(src), (size_t)0);
    CHECK_NULL(cdsa_dlist_head(src));

    int buf[6] = {0};
    CHECK_EQ(collect(dest, buf, 6), (size_t)6);
    for (int i = 0; i < 6; ++i) {
        CHECK_EQ(buf[i], i + 1);
    }

    cdsa_dlist_destroy(dest);
    cdsa_dlist_destroy(src);
}

static void test_splice_before_head(void)
{
    cdsa_dlist_t *dest = NULL;
    cdsa_dlist_t *src  = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &dest), CDSA_OK);
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &src),  CDSA_OK);

    for (int i = 4; i <= 6; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(dest, &i), CDSA_OK);
    }
    for (int i = 1; i <= 3; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(src, &i), CDSA_OK);
    }

    /* Splice src before the head of dest. */
    cdsa_dlist_node_t *head = cdsa_dlist_head(dest);
    CHECK_EQ(cdsa_dlist_splice(dest, head, src), CDSA_OK);
    CHECK_EQ(cdsa_dlist_len(dest), (size_t)6);
    CHECK_EQ(cdsa_dlist_len(src), (size_t)0);

    int buf[6] = {0};
    CHECK_EQ(collect(dest, buf, 6), (size_t)6);
    for (int i = 0; i < 6; ++i) {
        CHECK_EQ(buf[i], i + 1);
    }

    cdsa_dlist_destroy(dest);
    cdsa_dlist_destroy(src);
}

static void test_splice_before_middle(void)
{
    cdsa_dlist_t *dest = NULL;
    cdsa_dlist_t *src  = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &dest), CDSA_OK);
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &src),  CDSA_OK);

    /* dest = [1, 5], src = [2, 3, 4] → result [1,2,3,4,5] */
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(dest, &v), CDSA_OK);
    v = 5;
    CHECK_EQ(cdsa_dlist_push_back(dest, &v), CDSA_OK);
    for (int i = 2; i <= 4; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(src, &i), CDSA_OK);
    }

    cdsa_dlist_node_t *n5 = cdsa_dlist_tail(dest);
    CHECK_EQ(cdsa_dlist_splice(dest, n5, src), CDSA_OK);
    CHECK_EQ(cdsa_dlist_len(dest), (size_t)5);

    int buf[5] = {0};
    CHECK_EQ(collect(dest, buf, 5), (size_t)5);
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(buf[i], i + 1);
    }

    cdsa_dlist_destroy(dest);
    cdsa_dlist_destroy(src);
}

static void test_splice_empty_src(void)
{
    cdsa_dlist_t *dest = NULL;
    cdsa_dlist_t *src  = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &dest), CDSA_OK);
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &src),  CDSA_OK);

    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(dest, &v), CDSA_OK);
    CHECK_EQ(cdsa_dlist_splice(dest, NULL, src), CDSA_OK);
    CHECK_EQ(cdsa_dlist_len(dest), (size_t)1);

    cdsa_dlist_destroy(dest);
    cdsa_dlist_destroy(src);
}

static void test_splice_into_empty_dest(void)
{
    cdsa_dlist_t *dest = NULL;
    cdsa_dlist_t *src  = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &dest), CDSA_OK);
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &src),  CDSA_OK);

    for (int i = 1; i <= 3; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(src, &i), CDSA_OK);
    }
    CHECK_EQ(cdsa_dlist_splice(dest, NULL, src), CDSA_OK);
    CHECK_EQ(cdsa_dlist_len(dest), (size_t)3);
    CHECK_EQ(cdsa_dlist_len(src), (size_t)0);

    int buf[3] = {0};
    CHECK_EQ(collect(dest, buf, 3), (size_t)3);
    for (int i = 0; i < 3; ++i) {
        CHECK_EQ(buf[i], i + 1);
    }

    cdsa_dlist_destroy(dest);
    cdsa_dlist_destroy(src);
}

static void test_splice_inval(void)
{
    cdsa_dlist_t *a = NULL;
    cdsa_dlist_t *b = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &a), CDSA_OK);
    CHECK_EQ(cdsa_dlist_create(sizeof(long), NULL, NULL, &b), CDSA_OK);

    CHECK_EQ(cdsa_dlist_splice(NULL, NULL, a), CDSA_ERR_INVAL);
    CHECK_EQ(cdsa_dlist_splice(a, NULL, NULL), CDSA_ERR_INVAL);
    CHECK_EQ(cdsa_dlist_splice(a, NULL, a), CDSA_ERR_INVAL); /* same list */
    CHECK_EQ(cdsa_dlist_splice(a, NULL, b), CDSA_ERR_INVAL); /* elem_size mismatch */

    cdsa_dlist_destroy(a);
    cdsa_dlist_destroy(b);
}

/* ── cdsa_dlist_reverse ──────────────────────────────────────────────────── */

static void test_reverse_basic(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 5; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }
    cdsa_dlist_reverse(list);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)5);

    int buf[5] = {0};
    CHECK_EQ(collect(list, buf, 5), (size_t)5);
    CHECK_EQ(buf[0], 5);
    CHECK_EQ(buf[1], 4);
    CHECK_EQ(buf[2], 3);
    CHECK_EQ(buf[3], 2);
    CHECK_EQ(buf[4], 1);

    /* Reverse again restores original order. */
    cdsa_dlist_reverse(list);
    CHECK_EQ(collect(list, buf, 5), (size_t)5);
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(buf[i], i + 1);
    }
    cdsa_dlist_destroy(list);
}

static void test_reverse_empty(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    cdsa_dlist_reverse(list); /* must not crash */
    CHECK_EQ(cdsa_dlist_len(list), (size_t)0);
    cdsa_dlist_destroy(list);
}

static void test_reverse_single(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 42;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    cdsa_dlist_reverse(list);
    int out = 0;
    CHECK_EQ(cdsa_dlist_peek_front(list, &out), CDSA_OK);
    CHECK_EQ(out, 42);
    CHECK_EQ(cdsa_dlist_len(list), (size_t)1);
    cdsa_dlist_destroy(list);
}

static void test_reverse_two_elements(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    int v = 1;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    v = 2;
    CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
    cdsa_dlist_reverse(list);

    int buf[2] = {0};
    CHECK_EQ(collect(list, buf, 2), (size_t)2);
    CHECK_EQ(buf[0], 2);
    CHECK_EQ(buf[1], 1);

    /* Tail pointer must be updated correctly. */
    int out = 0;
    CHECK_EQ(cdsa_dlist_peek_back(list, &out), CDSA_OK);
    CHECK_EQ(out, 1);
    cdsa_dlist_destroy(list);
}

static void test_reverse_null(void)
{
    cdsa_dlist_reverse(NULL); /* must not crash */
}

/* Reverse then iterate backward — should give the original forward order. */
static void test_reverse_then_backward(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 4; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }
    cdsa_dlist_reverse(list);

    int buf[4] = {0};
    CHECK_EQ(collect_rev(list, buf, 4), (size_t)4);
    for (int i = 0; i < 4; ++i) {
        CHECK_EQ(buf[i], i + 1); /* 1 2 3 4 */
    }
    cdsa_dlist_destroy(list);
}

/* ── FIFO / LIFO / mixed ─────────────────────────────────────────────────── */

static void test_fifo_via_push_back_pop_front(void)
{
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), NULL, NULL, &list), CDSA_OK);
    for (int i = 1; i <= 5; ++i) {
        CHECK_EQ(cdsa_dlist_push_back(list, &i), CDSA_OK);
    }
    for (int expected = 1; expected <= 5; ++expected) {
        int out = 0;
        CHECK_EQ(cdsa_dlist_pop_front(list, &out), CDSA_OK);
        CHECK_EQ(out, expected);
    }
    CHECK_EQ(cdsa_dlist_pop_front(list, NULL), CDSA_ERR_EMPTY);
    cdsa_dlist_destroy(list);
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

static void ref_reverse(rtest_ref_t *r)
{
    for (size_t lo = 0u, hi = r->len - 1u; lo < hi; ++lo, --hi) {
        int tmp      = r->data[lo];
        r->data[lo]  = r->data[hi];
        r->data[hi]  = tmp;
    }
}

static void rtest_check(cdsa_dlist_t *list, const rtest_ref_t *r)
{
    if (cdsa_dlist_len(list) != r->len) {
        cdsa_check_failures++;
        return;
    }
    size_t k = 0u;
    cdsa_dlist_node_t *node = NULL;
    CDSA_DLIST_FOREACH(list, node) {
        CHECK_EQ(*(int *)cdsa_dlist_node_data(node), r->data[k]);
        ++k;
    }
}

static void test_rand_diff(void)
{
    rtest_ref_t  ref  = {{0}, 0u};
    cdsa_dlist_t *list = NULL;
    CHECK_EQ(cdsa_dlist_create(sizeof(int), cdsa_cmp_int, NULL, &list),
             CDSA_OK);
    if (list == NULL) {
        return;
    }

    g_rng = 42u;

    for (int iter = 0; iter < RTEST_OPS; ++iter) {
        uint32_t rng = rng_next();
        int      v   = (int)(rng_next() % 32u);

        switch (rng % 5u) {
        case 0u:
            if (ref.len < (size_t)RTEST_MAX) {
                CHECK_EQ(cdsa_dlist_push_front(list, &v), CDSA_OK);
                ref_push_front(&ref, v);
            }
            break;
        case 1u:
            if (ref.len > 0u) {
                CHECK_EQ(cdsa_dlist_pop_front(list, NULL), CDSA_OK);
                ref_pop_front(&ref);
            } else {
                CHECK_EQ(cdsa_dlist_pop_front(list, NULL), CDSA_ERR_EMPTY);
            }
            break;
        case 2u:
            if (ref.len < (size_t)RTEST_MAX) {
                CHECK_EQ(cdsa_dlist_push_back(list, &v), CDSA_OK);
                ref_push_back(&ref, v);
            }
            break;
        case 3u:
            if (ref.len > 0u) {
                CHECK_EQ(cdsa_dlist_pop_back(list, NULL), CDSA_OK);
                ref_pop_back(&ref);
            } else {
                CHECK_EQ(cdsa_dlist_pop_back(list, NULL), CDSA_ERR_EMPTY);
            }
            break;
        case 4u:
            if (ref.len > 0u) {
                cdsa_dlist_reverse(list);
                ref_reverse(&ref);
            }
            break;
        default:
            break;
        }

        rtest_check(list, &ref);
    }

    cdsa_dlist_destroy(list);
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
    /* push_front / pop_front */
    CDSA_T(test_push_front_basic),
    CDSA_T(test_pop_front_basic),
    CDSA_T(test_pop_front_null_list),
    CDSA_T(test_pop_front_alloc_failure),
    /* push_back / pop_back */
    CDSA_T(test_push_pop_back_basic),
    CDSA_T(test_pop_back_single_element),
    CDSA_T(test_pop_back_calls_free_fn),
    /* peek */
    CDSA_T(test_peek_front_back),
    CDSA_T(test_peek_empty),
    /* insert_before */
    CDSA_T(test_insert_before_middle),
    CDSA_T(test_insert_before_head),
    CDSA_T(test_insert_before_null_args),
    CDSA_T(test_insert_before_alloc_failure),
    /* insert_after */
    CDSA_T(test_insert_after_middle),
    CDSA_T(test_insert_after_tail),
    CDSA_T(test_insert_after_null_args),
    CDSA_T(test_insert_before_after_symmetry),
    /* remove_node */
    CDSA_T(test_remove_node_middle),
    CDSA_T(test_remove_node_head),
    CDSA_T(test_remove_node_tail),
    CDSA_T(test_remove_node_null_args),
    CDSA_T(test_remove_node_calls_free_fn),
    /* find / remove */
    CDSA_T(test_find_basic),
    CDSA_T(test_find_not_found),
    CDSA_T(test_remove_basic),
    /* bidirectional traversal */
    CDSA_T(test_node_prev_basic),
    CDSA_T(test_node_prev_null),
    CDSA_T(test_node_next_null),
    /* splice */
    CDSA_T(test_splice_append),
    CDSA_T(test_splice_before_head),
    CDSA_T(test_splice_before_middle),
    CDSA_T(test_splice_empty_src),
    CDSA_T(test_splice_into_empty_dest),
    CDSA_T(test_splice_inval),
    /* reverse */
    CDSA_T(test_reverse_basic),
    CDSA_T(test_reverse_empty),
    CDSA_T(test_reverse_single),
    CDSA_T(test_reverse_two_elements),
    CDSA_T(test_reverse_null),
    CDSA_T(test_reverse_then_backward),
    /* fifo/lifo */
    CDSA_T(test_fifo_via_push_back_pop_front),
    /* randomized */
    CDSA_T(test_rand_diff),
};

int main(void)
{
    return cdsa_run_suite("cdsa_dlist", tests, CDSA_ARRAY_LEN(tests));
}
