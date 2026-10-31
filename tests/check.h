/*
 * check.h — minimal TAP-output test framework for libcdsa.
 *
 * Usage (one .c file per test executable):
 *
 *   #include "check.h"
 *
 *   static void test_something(void) {
 *       CHECK_EQ(foo(), 42);
 *       CHECK_NOTNULL(bar());
 *   }
 *
 *   static const cdsa_test_entry_t tests[] = {
 *       CDSA_T(test_something),
 *   };
 *
 *   int main(void) {
 *       return cdsa_run_suite("my_module", tests, CDSA_ARRAY_LEN(tests));
 *   }
 */

#ifndef LIBCDSA_TESTS_CHECK_H
#define LIBCDSA_TESTS_CHECK_H

#include <stddef.h>
#include <stdio.h>

/* ── Internal state ──────────────────────────────────────────────────────────
 *
 * Count of CHECK failures in the currently-executing test function.
 * cdsa_run_suite resets this before each test and inspects it afterwards.
 * Tests that verify failure-detection behaviour may read/write this directly
 * (see test_check.c for an example).
 */
static int cdsa_check_failures = 0; /* NOLINT(misc-definitions-in-headers) */

/* ── Check macros ────────────────────────────────────────────────────────────
 *
 * On failure: print a TAP YAML diagnostic block to stderr and increment
 * cdsa_check_failures.  Execution of the test function continues (non-fatal).
 *
 * The CDSA_CHECK_* names are canonical; the CHECK_* short aliases are
 * provided for readability inside test files.
 */

#define CDSA_CHECK(expr_)                                                  \
    do {                                                                    \
        if (!(expr_)) {                                                     \
            fprintf(stderr,                                                 \
                    "  ---\n"                                               \
                    "  message: CHECK(%s) failed\n"                        \
                    "  at: %s:%d\n"                                        \
                    "  ...\n",                                              \
                    #expr_, __FILE__, __LINE__);                            \
            cdsa_check_failures++;                                          \
        }                                                                   \
    } while (0)

#define CDSA_CHECK_EQ(a_, b_)     CDSA_CHECK((a_) == (b_))
#define CDSA_CHECK_NE(a_, b_)     CDSA_CHECK((a_) != (b_))
#define CDSA_CHECK_LT(a_, b_)     CDSA_CHECK((a_) <  (b_))
#define CDSA_CHECK_LE(a_, b_)     CDSA_CHECK((a_) <= (b_))
#define CDSA_CHECK_GT(a_, b_)     CDSA_CHECK((a_) >  (b_))
#define CDSA_CHECK_GE(a_, b_)     CDSA_CHECK((a_) >= (b_))
#define CDSA_CHECK_NULL(p_)       CDSA_CHECK((p_) == NULL)
#define CDSA_CHECK_NOTNULL(p_)    CDSA_CHECK((p_) != NULL)

#define CHECK(e_)           CDSA_CHECK(e_)
#define CHECK_EQ(a_, b_)    CDSA_CHECK_EQ(a_, b_)
#define CHECK_NE(a_, b_)    CDSA_CHECK_NE(a_, b_)
#define CHECK_LT(a_, b_)    CDSA_CHECK_LT(a_, b_)
#define CHECK_LE(a_, b_)    CDSA_CHECK_LE(a_, b_)
#define CHECK_GT(a_, b_)    CDSA_CHECK_GT(a_, b_)
#define CHECK_GE(a_, b_)    CDSA_CHECK_GE(a_, b_)
#define CHECK_NULL(p_)      CDSA_CHECK_NULL(p_)
#define CHECK_NOTNULL(p_)   CDSA_CHECK_NOTNULL(p_)

/* ── Test registration ───────────────────────────────────────────────────── */

typedef void (*cdsa_test_fn_t)(void);

typedef struct {
    const char     *name;
    cdsa_test_fn_t  fn;
} cdsa_test_entry_t;

/* Build a cdsa_test_entry_t initialiser from a function name. */
#define CDSA_T(fn_)  { #fn_, (fn_) }

/* Number of elements in a compile-time array. */
#define CDSA_ARRAY_LEN(a_)  (sizeof(a_) / sizeof((a_)[0]))

/* ── Suite runner ────────────────────────────────────────────────────────────
 *
 * cdsa_run_suite — run every test in @tests and emit TAP 14 output to stdout.
 *
 * Per-test failure diagnostics go to stderr so they do not corrupt the TAP
 * stream on stdout.
 *
 * @suite_name  Label in the trailing summary comment (NULL = omit summary).
 * @tests       Array of cdsa_test_entry_t.
 * @n           Number of entries in @tests.
 * @return      0 if all tests passed, 1 if any test failed.
 *              Suitable as the return value of main().
 *
 * O(n) time.
 */
static int cdsa_run_suite(const char          *suite_name,
                           const cdsa_test_entry_t *tests,
                           size_t               n)
{
    printf("TAP version 14\n1..%zu\n", n);
    size_t passed = 0;
    for (size_t i = 0; i < n; ++i) {
        cdsa_check_failures = 0;
        tests[i].fn();
        if (cdsa_check_failures == 0) {
            printf("ok %zu - %s\n", i + 1, tests[i].name);
            ++passed;
        } else {
            printf("not ok %zu - %s\n", i + 1, tests[i].name);
        }
    }
    if (suite_name != NULL) {
        printf("# %s: %zu/%zu passed\n", suite_name, passed, n);
    }
    return (passed == n) ? 0 : 1;
}

#endif /* LIBCDSA_TESTS_CHECK_H */
