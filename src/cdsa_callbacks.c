#include "cdsa/cdsa_callbacks.h"
#include "cdsa/cdsa_alloc.h"

#include <stdint.h>
#include <string.h>

/* ── int ─────────────────────────────────────────────────────────────────── */

int cdsa_cmp_int(const void *a, const void *b)
{
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return (ia > ib) - (ia < ib);
}

uint64_t cdsa_hash_int(const void *key, size_t key_size)
{
    (void)key_size;
    /* Zero-extend through unsigned int to avoid UB on negative values. */
    uint64_t v = (uint64_t)(unsigned int)(*(const int *)key);
    /* MurmurHash3 64-bit finalizer — good avalanche for small integers. */
    v ^= v >> 33;
    v *= UINT64_C(0xff51afd7ed558ccd);
    v ^= v >> 33;
    v *= UINT64_C(0xc4ceb9fe1a85ec53);
    v ^= v >> 33;
    return v;
}

/* ── C string ────────────────────────────────────────────────────────────── */

int cdsa_cmp_str(const void *a, const void *b)
{
    const char *sa = *(const char *const *)a;
    const char *sb = *(const char *const *)b;
    return strcmp(sa, sb);
}

uint64_t cdsa_hash_str(const void *key, size_t key_size)
{
    (void)key_size;
    const char *s = *(const char *const *)key;
    /* FNV-1a 64-bit. */
    uint64_t h = UINT64_C(14695981039346656037);
    while (*s != '\0') {
        h ^= (uint64_t)(unsigned char)*s++;
        h *= UINT64_C(1099511628211);
    }
    return h;
}

void cdsa_free_str(void *elem)
{
    char **pp = (char **)elem;
    cdsa_free(*pp);
    *pp = NULL;
}

cdsa_status_t cdsa_copy_str(void *dst, const void *src, size_t elem_size)
{
    (void)elem_size;
    const char *s  = *(const char *const *)src;
    size_t      len = strlen(s);
    char       *copy = (char *)cdsa_malloc(len + 1u);
    if (copy == NULL) {
        *(char **)dst = NULL;
        return CDSA_ERR_NOMEM;
    }
    memcpy(copy, s, len + 1u);
    *(char **)dst = copy;
    return CDSA_OK;
}

/* ── Pointer identity ────────────────────────────────────────────────────── */

int cdsa_cmp_ptr(const void *a, const void *b)
{
    uintptr_t pa = (uintptr_t)(*(const void *const *)a);
    uintptr_t pb = (uintptr_t)(*(const void *const *)b);
    return (pa > pb) - (pa < pb);
}

uint64_t cdsa_hash_ptr(const void *key, size_t key_size)
{
    (void)key_size;
    uint64_t v = (uint64_t)(uintptr_t)(*(const void *const *)key);
    v ^= v >> 33;
    v *= UINT64_C(0xff51afd7ed558ccd);
    v ^= v >> 33;
    v *= UINT64_C(0xc4ceb9fe1a85ec53);
    v ^= v >> 33;
    return v;
}

/* ── Shared no-op / shallow ─────────────────────────────────────────────── */

void cdsa_free_noop(void *elem)
{
    (void)elem;
}

cdsa_status_t cdsa_copy_memcpy(void *dst, const void *src, size_t elem_size)
{
    memcpy(dst, src, elem_size);
    return CDSA_OK;
}
