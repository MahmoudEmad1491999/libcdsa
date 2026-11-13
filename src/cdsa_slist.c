#include "cdsa/cdsa_slist.h"

#include <string.h>

#include "cdsa/cdsa_alloc.h"

struct cdsa_slist_node {
    struct cdsa_slist_node *next;
    /* elem_size bytes of element data follow in memory */
};

struct cdsa_slist {
    cdsa_slist_node_t  *head;      /* sentinel — data area unused */
    cdsa_slist_node_t  *tail;      /* last real node, or head when empty */
    size_t              len;
    size_t              elem_size;
    cdsa_cmp_fn         cmp_fn;
    cdsa_free_fn        free_fn;
};

/* Return a pointer to the element data stored just after the node header. */
static void *node_data(cdsa_slist_node_t *node)
{
    return (void *)((char *)node + sizeof(cdsa_slist_node_t));
}

/* Allocate a new node (header + elem_size bytes).  Returns NULL on failure. */
static cdsa_slist_node_t *slist_alloc_node(size_t elem_size)
{
    cdsa_slist_node_t *n =
        (cdsa_slist_node_t *)cdsa_malloc(sizeof(cdsa_slist_node_t) + elem_size);
    if (n != NULL) {
        n->next = NULL;
    }
    return n;
}

cdsa_status_t cdsa_slist_create(size_t elem_size, cdsa_cmp_fn cmp_fn,
                                 cdsa_free_fn free_fn, cdsa_slist_t **out)
{
    if (elem_size == 0u || out == NULL) {
        return CDSA_ERR_INVAL;
    }

    cdsa_slist_t *list = (cdsa_slist_t *)cdsa_malloc(sizeof(cdsa_slist_t));
    if (list == NULL) {
        return CDSA_ERR_NOMEM;
    }

    list->head = slist_alloc_node(elem_size);
    if (list->head == NULL) {
        cdsa_free(list);
        return CDSA_ERR_NOMEM;
    }

    list->tail      = list->head;
    list->len       = 0u;
    list->elem_size = elem_size;
    list->cmp_fn    = cmp_fn;
    list->free_fn   = free_fn;

    *out = list;
    return CDSA_OK;
}

void cdsa_slist_destroy(cdsa_slist_t *list)
{
    if (list == NULL) {
        return;
    }

    cdsa_slist_node_t *cur = list->head->next;
    while (cur != NULL) {
        cdsa_slist_node_t *nxt = cur->next;
        if (list->free_fn != NULL) {
            list->free_fn(node_data(cur));
        }
        cdsa_free(cur);
        cur = nxt;
    }

    cdsa_free(list->head);
    cdsa_free(list);
}

cdsa_status_t cdsa_slist_push_front(cdsa_slist_t *list, const void *elem)
{
    if (list == NULL || elem == NULL) {
        return CDSA_ERR_INVAL;
    }

    cdsa_slist_node_t *n = slist_alloc_node(list->elem_size);
    if (n == NULL) {
        return CDSA_ERR_NOMEM;
    }

    memcpy(node_data(n), elem, list->elem_size);
    n->next          = list->head->next;
    list->head->next = n;
    if (list->tail == list->head) {   /* was empty */
        list->tail = n;
    }
    list->len++;
    return CDSA_OK;
}

cdsa_status_t cdsa_slist_pop_front(cdsa_slist_t *list, void *out)
{
    if (list == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->head->next == NULL) {
        return CDSA_ERR_EMPTY;
    }

    cdsa_slist_node_t *n = list->head->next;
    list->head->next = n->next;
    if (n->next == NULL) {            /* list is now empty */
        list->tail = list->head;
    }

    if (out != NULL) {
        memcpy(out, node_data(n), list->elem_size);
    } else if (list->free_fn != NULL) {
        list->free_fn(node_data(n));
    }

    cdsa_free(n);
    list->len--;
    return CDSA_OK;
}

cdsa_status_t cdsa_slist_push_back(cdsa_slist_t *list, const void *elem)
{
    if (list == NULL || elem == NULL) {
        return CDSA_ERR_INVAL;
    }

    cdsa_slist_node_t *n = slist_alloc_node(list->elem_size);
    if (n == NULL) {
        return CDSA_ERR_NOMEM;
    }

    memcpy(node_data(n), elem, list->elem_size);
    list->tail->next = n;
    list->tail       = n;
    list->len++;
    return CDSA_OK;
}

cdsa_status_t cdsa_slist_pop_back(cdsa_slist_t *list, void *out)
{
    if (list == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->head->next == NULL) {
        return CDSA_ERR_EMPTY;
    }

    /* Walk to the second-to-last node (starting from sentinel). */
    cdsa_slist_node_t *prev = list->head;
    while (prev->next != list->tail) {
        prev = prev->next;
    }

    cdsa_slist_node_t *n = list->tail;

    if (out != NULL) {
        memcpy(out, node_data(n), list->elem_size);
    } else if (list->free_fn != NULL) {
        list->free_fn(node_data(n));
    }

    prev->next = NULL;
    list->tail = prev;
    cdsa_free(n);
    list->len--;
    return CDSA_OK;
}

cdsa_status_t cdsa_slist_peek_front(const cdsa_slist_t *list, void *out)
{
    if (list == NULL || out == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->head->next == NULL) {
        return CDSA_ERR_EMPTY;
    }
    memcpy(out, node_data(list->head->next), list->elem_size);
    return CDSA_OK;
}

cdsa_status_t cdsa_slist_peek_back(const cdsa_slist_t *list, void *out)
{
    if (list == NULL || out == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->tail == list->head) {
        return CDSA_ERR_EMPTY;
    }
    memcpy(out, node_data(list->tail), list->elem_size);
    return CDSA_OK;
}

cdsa_status_t cdsa_slist_find(const cdsa_slist_t *list, const void *key,
                               cdsa_slist_node_t **node_out)
{
    if (list == NULL || key == NULL || node_out == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->cmp_fn == NULL) {
        return CDSA_ERR_INVAL;
    }

    cdsa_slist_node_t *cur = list->head->next;
    while (cur != NULL) {
        if (list->cmp_fn(node_data(cur), key) == 0) {
            *node_out = cur;
            return CDSA_OK;
        }
        cur = cur->next;
    }
    return CDSA_ERR_NOTFOUND;
}

cdsa_status_t cdsa_slist_remove(cdsa_slist_t *list, const void *key, void *out)
{
    if (list == NULL || key == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->cmp_fn == NULL) {
        return CDSA_ERR_INVAL;
    }

    cdsa_slist_node_t *prev = list->head;
    cdsa_slist_node_t *cur  = list->head->next;

    while (cur != NULL) {
        if (list->cmp_fn(node_data(cur), key) == 0) {
            prev->next = cur->next;
            if (cur == list->tail) {
                list->tail = prev;
            }
            if (out != NULL) {
                memcpy(out, node_data(cur), list->elem_size);
            } else if (list->free_fn != NULL) {
                list->free_fn(node_data(cur));
            }
            cdsa_free(cur);
            list->len--;
            return CDSA_OK;
        }
        prev = cur;
        cur  = cur->next;
    }
    return CDSA_ERR_NOTFOUND;
}

size_t cdsa_slist_len(const cdsa_slist_t *list)
{
    return (list != NULL) ? list->len : 0u;
}

cdsa_slist_node_t *cdsa_slist_head(const cdsa_slist_t *list)
{
    return (list != NULL) ? list->head->next : NULL;
}

cdsa_slist_node_t *cdsa_slist_node_next(cdsa_slist_node_t *node)
{
    return (node != NULL) ? node->next : NULL;
}

void *cdsa_slist_node_data(cdsa_slist_node_t *node)
{
    return (node != NULL) ? node_data(node) : NULL;
}
