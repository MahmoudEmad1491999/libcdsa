#include "check.h"

#include <stddef.h>
#include <stdlib.h>

/* ── Helpers used by failure-detection test ────────────────────────────── */

static void sub_check_false(void)
{
    CHECK(0 == 1); /* always false — used to verify failure detection below */
}

/* ── Tests ───────────────────────────────────────────────────────────────── */

static void test_check_passes_on_true(void)
{
    CHECK(1 == 1);
    CHECK(!(0));
}

static void test_check_eq_ne(void)
{
    CHECK_EQ(0, 0);
    CHECK_EQ(42, 42);
    CHECK_EQ(-1, -1);
    CHECK_NE(1, 2);
    CHECK_NE(-1, 0);
}

static void test_check_relational(void)
{
    CHECK_LT(1, 2);
    CHECK_LE(1, 2);
    CHECK_LE(2, 2);
    CHECK_GT(3, 2);
    CHECK_GE(3, 2);
    CHECK_GE(3, 3);
}

static void test_check_null_notnull(void)
{
    void *null_ptr = NULL;
    int   dummy    = 0;
    void *nonnull  = &dummy;
    CHECK_NULL(null_ptr);
    CHECK_NULL(NULL);
    CHECK_NOTNULL(nonnull);
}

static void test_cdsa_array_len(void)
{
    static const int arr[7] = {0};
    size_t len = CDSA_ARRAY_LEN(arr);
    CHECK_EQ(len, (size_t)7);
}

static void test_failure_detection(void)
{
    /*
     * Verify that CHECK increments cdsa_check_failures when the condition is
     * false.  We save the counter, call a helper that always fails, confirm
     * the increment, then restore the counter so this test is not itself
     * marked as failed by the runner.
     *
     * The diagnostic printed to stderr by sub_check_false is intentional.
     */
    int before = cdsa_check_failures;
    fprintf(stderr, "# (deliberate CHECK failure follows — expected)\n");
    sub_check_false();                        /* increments cdsa_check_failures */
    CHECK_EQ(cdsa_check_failures, before + 1);
    cdsa_check_failures = before;             /* undo: this test should pass */
}

/* ── Main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    static const cdsa_test_entry_t tests[] = {
        CDSA_T(test_check_passes_on_true),
        CDSA_T(test_check_eq_ne),
        CDSA_T(test_check_relational),
        CDSA_T(test_check_null_notnull),
        CDSA_T(test_cdsa_array_len),
        CDSA_T(test_failure_detection),
    };
    return cdsa_run_suite("check", tests, CDSA_ARRAY_LEN(tests));
}
