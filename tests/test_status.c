#include "cdsa/cdsa_status.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void test_ok_is_zero(void)
{
    assert(CDSA_OK == 0);
}

static void test_errors_are_negative(void)
{
    assert(CDSA_ERR_NOMEM < 0);
    assert(CDSA_ERR_INVAL < 0);
    assert(CDSA_ERR_NOTFOUND < 0);
    assert(CDSA_ERR_EMPTY < 0);
    assert(CDSA_ERR_FULL < 0);
    assert(CDSA_ERR_RANGE < 0);
    assert(CDSA_ERR_DUPLICATE < 0);
}

static void test_errors_are_distinct(void)
{
    const int codes[] = {
        (int)CDSA_ERR_NOMEM,    (int)CDSA_ERR_INVAL,  (int)CDSA_ERR_NOTFOUND,
        (int)CDSA_ERR_EMPTY,    (int)CDSA_ERR_FULL,   (int)CDSA_ERR_RANGE,
        (int)CDSA_ERR_DUPLICATE
    };
    const size_t n = sizeof(codes) / sizeof(codes[0]);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            assert(codes[i] != codes[j]);
        }
    }
}

static void test_strerror_ok(void)
{
    /* No intermediate variable: works in both Debug and Release (-DNDEBUG). */
    assert(cdsa_strerror(CDSA_OK) != NULL);
    assert(strlen(cdsa_strerror(CDSA_OK)) > 0);
}

static void test_strerror_known_errors(void)
{
    static const cdsa_status_t known[] = {
        CDSA_ERR_NOMEM,    CDSA_ERR_INVAL,  CDSA_ERR_NOTFOUND,
        CDSA_ERR_EMPTY,    CDSA_ERR_FULL,   CDSA_ERR_RANGE,
        CDSA_ERR_DUPLICATE
    };
    const size_t n = sizeof(known) / sizeof(known[0]);
    for (size_t i = 0; i < n; ++i) {
        const char *s = cdsa_strerror(known[i]);
        assert(s != NULL);
        assert(strlen(s) > 0);
        assert(strcmp(s, "CDSA_ERR_UNKNOWN") != 0);
        (void)s; /* suppress unused-variable in -DNDEBUG builds */
    }
}

static void test_strerror_unknown_value(void)
{
    const char *s = cdsa_strerror((cdsa_status_t)-999);
    assert(s != NULL);
    assert(strcmp(s, "CDSA_ERR_UNKNOWN") == 0);
    (void)s;
}

static void test_strerror_returns_static_storage(void)
{
    const char *a = cdsa_strerror(CDSA_ERR_NOMEM);
    const char *b = cdsa_strerror(CDSA_ERR_NOMEM);
    assert(a == b);
    (void)a;
    (void)b;
}

int main(void)
{
    test_ok_is_zero();
    test_errors_are_negative();
    test_errors_are_distinct();
    test_strerror_ok();
    test_strerror_known_errors();
    test_strerror_unknown_value();
    test_strerror_returns_static_storage();
    return EXIT_SUCCESS;
}
