#include "check.h"
#include "cdsa/cdsa_callbacks.h"
#include "cdsa/cdsa_alloc.h"
#include "cdsa/cdsa_status.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
 * cdsa_cmp_int
 * ═════════════════════════════════════════════════════════════════════════ */

static void test_cmp_int_less(void)
{
    int a = 1, b = 2;
    CHECK_LT(cdsa_cmp_int(&a, &b), 0);
}

static void test_cmp_int_equal(void)
{
    int a = 42, b = 42;
    CHECK_EQ(cdsa_cmp_int(&a, &b), 0);
}

static void test_cmp_int_greater(void)
{
    int a = 5, b = 3;
    CHECK_GT(cdsa_cmp_int(&a, &b), 0);
}

static void test_cmp_int_negative_values(void)
{
    int a = -10, b = -5;
    CHECK_LT(cdsa_cmp_int(&a, &b), 0);
    int c = -5, d = -10;
    CHECK_GT(cdsa_cmp_int(&c, &d), 0);
}

static void test_cmp_int_min_max(void)
{
    /* Would overflow with naive subtraction — verify no overflow. */
    int a = -2147483647 - 1; /* INT_MIN */
    int b = 2147483647;      /* INT_MAX */
    CHECK_LT(cdsa_cmp_int(&a, &b), 0);
    CHECK_GT(cdsa_cmp_int(&b, &a), 0);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * cdsa_cmp_int used with qsort (integration)
 * ═════════════════════════════════════════════════════════════════════════ */

static void test_cmp_int_with_qsort(void)
{
    int arr[]      = {5, 3, 8, 1, 9, 2, 4, 7, 6, 0};
    int expected[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    size_t n = sizeof(arr) / sizeof(arr[0]);
    qsort(arr, n, sizeof(int), cdsa_cmp_int);
    for (size_t i = 0; i < n; ++i) {
        CHECK_EQ(arr[i], expected[i]);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * cdsa_hash_int
 * ═════════════════════════════════════════════════════════════════════════ */

static void test_hash_int_deterministic(void)
{
    int v = 12345;
    CHECK_EQ(cdsa_hash_int(&v, sizeof(v)), cdsa_hash_int(&v, sizeof(v)));
}

static void test_hash_int_different_values_differ(void)
{
    int a = 0, b = 1;
    CHECK_NE(cdsa_hash_int(&a, sizeof(a)), cdsa_hash_int(&b, sizeof(b)));
}

static void test_hash_int_negative(void)
{
    int a = -1, b = -2;
    CHECK_EQ(cdsa_hash_int(&a, sizeof(a)), cdsa_hash_int(&a, sizeof(a)));
    CHECK_NE(cdsa_hash_int(&a, sizeof(a)), cdsa_hash_int(&b, sizeof(b)));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * cdsa_cmp_str
 * ═════════════════════════════════════════════════════════════════════════ */

static void test_cmp_str_equal(void)
{
    const char *a = "hello";
    const char *b = "hello";
    CHECK_EQ(cdsa_cmp_str(&a, &b), 0);
}

static void test_cmp_str_less(void)
{
    const char *a = "apple";
    const char *b = "banana";
    CHECK_LT(cdsa_cmp_str(&a, &b), 0);
}

static void test_cmp_str_greater(void)
{
    const char *a = "zoo";
    const char *b = "ant";
    CHECK_GT(cdsa_cmp_str(&a, &b), 0);
}

static void test_cmp_str_empty(void)
{
    const char *a = "";
    const char *b = "";
    CHECK_EQ(cdsa_cmp_str(&a, &b), 0);
    const char *c = "a";
    CHECK_LT(cdsa_cmp_str(&a, &c), 0);
    CHECK_GT(cdsa_cmp_str(&c, &a), 0);
}

static void test_cmp_str_with_qsort(void)
{
    const char *arr[]      = {"cherry", "apple", "banana"};
    const char *expected[] = {"apple", "banana", "cherry"};
    size_t n = sizeof(arr) / sizeof(arr[0]);
    qsort(arr, n, sizeof(const char *), cdsa_cmp_str);
    for (size_t i = 0; i < n; ++i) {
        CHECK_EQ(strcmp(arr[i], expected[i]), 0);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * cdsa_hash_str
 * ═════════════════════════════════════════════════════════════════════════ */

static void test_hash_str_deterministic(void)
{
    const char *s = "test_string";
    CHECK_EQ(cdsa_hash_str(&s, 0), cdsa_hash_str(&s, 0));
}

static void test_hash_str_different_strings_differ(void)
{
    const char *a = "foo";
    const char *b = "bar";
    CHECK_NE(cdsa_hash_str(&a, 0), cdsa_hash_str(&b, 0));
}

static void test_hash_str_empty_equals_fnv_basis(void)
{
    /* FNV-1a of "" is the 64-bit offset basis — no loop iterations. */
    const char  *s        = "";
    uint64_t     expected = UINT64_C(14695981039346656037);
    CHECK_EQ(cdsa_hash_str(&s, 0), expected);
}

static void test_hash_str_key_size_ignored(void)
{
    /* key_size parameter must be ignored for string hashing. */
    const char *s = "ignored_size";
    CHECK_EQ(cdsa_hash_str(&s, 0), cdsa_hash_str(&s, 999));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * cdsa_copy_str / cdsa_free_str
 * ═════════════════════════════════════════════════════════════════════════ */

static void test_copy_str_basic(void)
{
    const char *src = "hello world";
    char       *dst = NULL;
    cdsa_status_t rc = cdsa_copy_str(&dst, &src, 0);
    CHECK_EQ(rc, CDSA_OK);
    CHECK_NOTNULL(dst);
    CHECK_EQ(strcmp(dst, src), 0);
    /* Must be a separate allocation, not the same pointer. */
    CHECK_NE((const void *)dst, (const void *)src);
    cdsa_free_str(&dst);
    CHECK_NULL(dst);
}

static void test_copy_str_empty_string(void)
{
    const char *src = "";
    char       *dst = NULL;
    cdsa_status_t rc = cdsa_copy_str(&dst, &src, 0);
    CHECK_EQ(rc, CDSA_OK);
    CHECK_NOTNULL(dst);
    CHECK_EQ(strcmp(dst, ""), 0);
    cdsa_free_str(&dst);
}

static void test_copy_str_alloc_failure(void)
{
    const char *src = "test";
    char       *dst = NULL;
    cdsa_alloc_fail_after(1);
    cdsa_status_t rc = cdsa_copy_str(&dst, &src, 0);
    CHECK_EQ(rc, CDSA_ERR_NOMEM);
    CHECK_NULL(dst); /* left in a safe state */
    cdsa_alloc_fail_after(0);
}

static void test_free_str_nulls_pointer(void)
{
    const char *src = "free me";
    char       *dst = NULL;
    CHECK_EQ(cdsa_copy_str(&dst, &src, 0), CDSA_OK);
    CHECK_NOTNULL(dst);
    cdsa_free_str(&dst);
    CHECK_NULL(dst);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * cdsa_cmp_ptr
 * ═════════════════════════════════════════════════════════════════════════ */

static void test_cmp_ptr_equal(void)
{
    int v    = 42;
    void *a  = &v;
    void *b  = &v;
    CHECK_EQ(cdsa_cmp_ptr(&a, &b), 0);
}

static void test_cmp_ptr_null_equal(void)
{
    void *a = NULL;
    void *b = NULL;
    CHECK_EQ(cdsa_cmp_ptr(&a, &b), 0);
}

static void test_cmp_ptr_adjacent_elements(void)
{
    /* arr[0] has lower address than arr[1] on all sane platforms. */
    int arr[2] = {0, 0};
    void *lo = &arr[0];
    void *hi = &arr[1];
    CHECK_LT(cdsa_cmp_ptr(&lo, &hi), 0);
    CHECK_GT(cdsa_cmp_ptr(&hi, &lo), 0);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * cdsa_hash_ptr
 * ═════════════════════════════════════════════════════════════════════════ */

static void test_hash_ptr_deterministic(void)
{
    int   v = 0;
    void *p = &v;
    CHECK_EQ(cdsa_hash_ptr(&p, 0), cdsa_hash_ptr(&p, 0));
}

static void test_hash_ptr_null(void)
{
    void *p = NULL;
    /* Must not crash; must be consistent. */
    CHECK_EQ(cdsa_hash_ptr(&p, 0), cdsa_hash_ptr(&p, 0));
}

static void test_hash_ptr_different_pointers_differ(void)
{
    int   a = 0, b = 0;
    void *pa = &a;
    void *pb = &b;
    /* Two distinct stack variables should hash differently. */
    CHECK_NE(cdsa_hash_ptr(&pa, 0), cdsa_hash_ptr(&pb, 0));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * cdsa_free_noop / cdsa_copy_memcpy
 * ═════════════════════════════════════════════════════════════════════════ */

static void test_free_noop_does_not_crash(void)
{
    int v = 99;
    cdsa_free_noop(&v);
    CHECK_EQ(v, 99); /* value untouched */
}

static void test_copy_memcpy_int(void)
{
    int src = 42, dst = 0;
    cdsa_status_t rc = cdsa_copy_memcpy(&dst, &src, sizeof(int));
    CHECK_EQ(rc, CDSA_OK);
    CHECK_EQ(dst, 42);
}

static void test_copy_memcpy_struct(void)
{
    struct { int x; int y; } src = {3, 7}, dst = {0, 0};
    cdsa_status_t rc = cdsa_copy_memcpy(&dst, &src, sizeof(src));
    CHECK_EQ(rc, CDSA_OK);
    CHECK_EQ(dst.x, 3);
    CHECK_EQ(dst.y, 7);
}

static void test_copy_memcpy_always_ok(void)
{
    /* cdsa_copy_memcpy never fails — verify it returns CDSA_OK. */
    char buf[16];
    const char fill[16] = "abcdefghijklmno";
    cdsa_status_t rc = cdsa_copy_memcpy(buf, fill, sizeof(buf));
    CHECK_EQ(rc, CDSA_OK);
    CHECK_EQ(memcmp(buf, fill, sizeof(buf)), 0);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Test registry
 * ═════════════════════════════════════════════════════════════════════════ */

static const cdsa_test_entry_t tests[] = {
    /* cmp_int */
    CDSA_T(test_cmp_int_less),
    CDSA_T(test_cmp_int_equal),
    CDSA_T(test_cmp_int_greater),
    CDSA_T(test_cmp_int_negative_values),
    CDSA_T(test_cmp_int_min_max),
    CDSA_T(test_cmp_int_with_qsort),
    /* hash_int */
    CDSA_T(test_hash_int_deterministic),
    CDSA_T(test_hash_int_different_values_differ),
    CDSA_T(test_hash_int_negative),
    /* cmp_str */
    CDSA_T(test_cmp_str_equal),
    CDSA_T(test_cmp_str_less),
    CDSA_T(test_cmp_str_greater),
    CDSA_T(test_cmp_str_empty),
    CDSA_T(test_cmp_str_with_qsort),
    /* hash_str */
    CDSA_T(test_hash_str_deterministic),
    CDSA_T(test_hash_str_different_strings_differ),
    CDSA_T(test_hash_str_empty_equals_fnv_basis),
    CDSA_T(test_hash_str_key_size_ignored),
    /* copy_str / free_str */
    CDSA_T(test_copy_str_basic),
    CDSA_T(test_copy_str_empty_string),
    CDSA_T(test_copy_str_alloc_failure),
    CDSA_T(test_free_str_nulls_pointer),
    /* cmp_ptr */
    CDSA_T(test_cmp_ptr_equal),
    CDSA_T(test_cmp_ptr_null_equal),
    CDSA_T(test_cmp_ptr_adjacent_elements),
    /* hash_ptr */
    CDSA_T(test_hash_ptr_deterministic),
    CDSA_T(test_hash_ptr_null),
    CDSA_T(test_hash_ptr_different_pointers_differ),
    /* free_noop / copy_memcpy */
    CDSA_T(test_free_noop_does_not_crash),
    CDSA_T(test_copy_memcpy_int),
    CDSA_T(test_copy_memcpy_struct),
    CDSA_T(test_copy_memcpy_always_ok),
};

int main(void)
{
    return cdsa_run_suite("cdsa_callbacks", tests, CDSA_ARRAY_LEN(tests));
}
