#include "cdsa/cdsa_vec.h"

#include <stdint.h>
#include <string.h>

#include "cdsa/cdsa_alloc.h"

/* Initial capacity allocated on the first push. */
#define CDSA_VEC_INITIAL_CAP ((size_t)4)

struct cdsa_vec {
    void        *data;
    size_t       len;
    size_t       cap;
    size_t       elem_size;
    cdsa_free_fn free_fn;
};

cdsa_status_t cdsa_vec_create(size_t elem_size, cdsa_free_fn free_fn,
                               cdsa_vec_t **out)
{
    if (elem_size == 0u || out == NULL) {
        return CDSA_ERR_INVAL;
    }

    cdsa_vec_t *v = cdsa_malloc(sizeof(cdsa_vec_t));
    if (v == NULL) {
        return CDSA_ERR_NOMEM;
    }

    v->data      = NULL;
    v->len       = 0u;
    v->cap       = 0u;
    v->elem_size = elem_size;
    v->free_fn   = free_fn;

    *out = v;
    return CDSA_OK;
}

void cdsa_vec_destroy(cdsa_vec_t *v)
{
    if (v == NULL) {
        return;
    }

    if (v->free_fn != NULL) {
        for (size_t i = 0u; i < v->len; ++i) {
            v->free_fn((char *)v->data + i * v->elem_size);
        }
    }

    cdsa_free(v->data);
    cdsa_free(v);
}

/* Grow the buffer to accommodate at least one more element. */
static cdsa_status_t vec_grow(cdsa_vec_t *v)
{
    size_t new_cap;
    if (v->cap == 0u) {
        new_cap = CDSA_VEC_INITIAL_CAP;
    } else if (v->cap > SIZE_MAX / (size_t)2) {
        return CDSA_ERR_NOMEM;
    } else {
        new_cap = v->cap * (size_t)2;
    }

    if (new_cap > SIZE_MAX / v->elem_size) {
        return CDSA_ERR_NOMEM;
    }

    void *new_data = cdsa_realloc(v->data, new_cap * v->elem_size);
    if (new_data == NULL) {
        return CDSA_ERR_NOMEM;
    }

    v->data = new_data;
    v->cap  = new_cap;
    return CDSA_OK;
}

cdsa_status_t cdsa_vec_push(cdsa_vec_t *v, const void *elem)
{
    if (v == NULL || elem == NULL) {
        return CDSA_ERR_INVAL;
    }

    if (v->len == v->cap) {
        cdsa_status_t rc = vec_grow(v);
        if (rc != CDSA_OK) {
            return rc;
        }
    }

    memcpy((char *)v->data + v->len * v->elem_size, elem, v->elem_size);
    ++v->len;
    return CDSA_OK;
}

cdsa_status_t cdsa_vec_pop(cdsa_vec_t *v, void *out)
{
    if (v == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (v->len == 0u) {
        return CDSA_ERR_EMPTY;
    }

    --v->len;
    void *slot = (char *)v->data + v->len * v->elem_size;

    if (out != NULL) {
        memcpy(out, slot, v->elem_size);
    } else if (v->free_fn != NULL) {
        v->free_fn(slot);
    }

    return CDSA_OK;
}

cdsa_status_t cdsa_vec_get(const cdsa_vec_t *v, size_t index, void **elem_out)
{
    if (v == NULL || elem_out == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (index >= v->len) {
        return CDSA_ERR_RANGE;
    }

    *elem_out = (char *)v->data + index * v->elem_size;
    return CDSA_OK;
}

cdsa_status_t cdsa_vec_set(cdsa_vec_t *v, size_t index, const void *elem)
{
    if (v == NULL || elem == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (index >= v->len) {
        return CDSA_ERR_RANGE;
    }

    void *slot = (char *)v->data + index * v->elem_size;

    if (v->free_fn != NULL) {
        v->free_fn(slot);
    }

    memcpy(slot, elem, v->elem_size);
    return CDSA_OK;
}

size_t cdsa_vec_len(const cdsa_vec_t *v)
{
    return (v != NULL) ? v->len : 0u;
}

size_t cdsa_vec_cap(const cdsa_vec_t *v)
{
    return (v != NULL) ? v->cap : 0u;
}
