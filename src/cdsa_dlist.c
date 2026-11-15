#include "cdsa/cdsa_dlist.h"

#include <string.h>

#include "cdsa/cdsa_alloc.h"

struct cdsa_dlist_node {
    struct cdsa_dlist_node *next;
    struct cdsa_dlist_node *prev; /* NULL only on the sentinel */
    /* elem_size bytes of element data follow in memory */
};

struct cdsa_dlist {
    cdsa_dlist_node_t *sent;      /* sentinel: sent->prev == NULL always */
    cdsa_dlist_node_t *tail;      /* last real node, or sent when empty */
    size_t             len;
    size_t             elem_size;
    cdsa_cmp_fn        cmp_fn;
    cdsa_free_fn       free_fn;
};

static void *node_data(cdsa_dlist_node_t *node)
{
    return (void *)((char *)node + sizeof(cdsa_dlist_node_t));
}

static cdsa_dlist_node_t *dlist_alloc_node(size_t elem_size)
{
    cdsa_dlist_node_t *n =
        (cdsa_dlist_node_t *)cdsa_malloc(sizeof(cdsa_dlist_node_t) + elem_size);
    if (n != NULL) {
        n->next = NULL;
        n->prev = NULL;
    }
    return n;
}

/* Insert a fully-initialised new_node right after `at` (which may be the
 * sentinel or any real node). Updates the tail pointer when necessary. */
static void dlist_link_after(cdsa_dlist_t *list, cdsa_dlist_node_t *at,
                              cdsa_dlist_node_t *new_node)
{
    new_node->prev = at;
    new_node->next = at->next;
    if (at->next != NULL) {
        at->next->prev = new_node;
    }
    at->next = new_node;
    if (list->tail == at) {
        list->tail = new_node;
    }
}

/* Unlink `node` from its neighbours; does NOT free it or update len. */
static void dlist_unlink(cdsa_dlist_t *list, cdsa_dlist_node_t *node)
{
    node->prev->next = node->next;
    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    if (list->tail == node) {
        list->tail = node->prev;
    }
}

/* ── Create / destroy ────────────────────────────────────────────────────── */

cdsa_status_t cdsa_dlist_create(size_t elem_size, cdsa_cmp_fn cmp_fn,
                                 cdsa_free_fn free_fn, cdsa_dlist_t **out)
{
    if (elem_size == 0u || out == NULL) {
        return CDSA_ERR_INVAL;
    }

    cdsa_dlist_t *list = (cdsa_dlist_t *)cdsa_malloc(sizeof(cdsa_dlist_t));
    if (list == NULL) {
        return CDSA_ERR_NOMEM;
    }

    /* Allocate the sentinel without element data — its data area is never read.
     * We pass 0 for elem_size here to avoid wasting elem_size bytes per list. */
    list->sent = (cdsa_dlist_node_t *)cdsa_malloc(sizeof(cdsa_dlist_node_t));
    if (list->sent == NULL) {
        cdsa_free(list);
        return CDSA_ERR_NOMEM;
    }

    list->sent->next = NULL;
    list->sent->prev = NULL; /* NULL prev identifies the sentinel */
    list->tail        = list->sent;
    list->len         = 0u;
    list->elem_size   = elem_size;
    list->cmp_fn      = cmp_fn;
    list->free_fn     = free_fn;

    *out = list;
    return CDSA_OK;
}

void cdsa_dlist_destroy(cdsa_dlist_t *list)
{
    if (list == NULL) {
        return;
    }

    cdsa_dlist_node_t *cur = list->sent->next;
    while (cur != NULL) {
        cdsa_dlist_node_t *nxt = cur->next;
        if (list->free_fn != NULL) {
            list->free_fn(node_data(cur));
        }
        cdsa_free(cur);
        cur = nxt;
    }

    cdsa_free(list->sent);
    cdsa_free(list);
}

/* ── Shared helpers ──────────────────────────────────────────────────────── */

/* Allocate, populate, and link a new node after `at`. */
static cdsa_status_t dlist_insert_after_node(cdsa_dlist_t *list,
                                              cdsa_dlist_node_t *at,
                                              const void *elem)
{
    cdsa_dlist_node_t *n = dlist_alloc_node(list->elem_size);
    if (n == NULL) {
        return CDSA_ERR_NOMEM;
    }
    memcpy(node_data(n), elem, list->elem_size);
    dlist_link_after(list, at, n);
    list->len++;
    return CDSA_OK;
}

/* Unlink, optionally copy or free, then free the node. */
static void dlist_remove_and_free(cdsa_dlist_t *list, cdsa_dlist_node_t *node,
                                   void *out)
{
    dlist_unlink(list, node);
    if (out != NULL) {
        memcpy(out, node_data(node), list->elem_size);
    } else if (list->free_fn != NULL) {
        list->free_fn(node_data(node));
    }
    cdsa_free(node);
    list->len--;
}

/* ── Push / pop front ────────────────────────────────────────────────────── */

cdsa_status_t cdsa_dlist_push_front(cdsa_dlist_t *list, const void *elem)
{
    if (list == NULL || elem == NULL) {
        return CDSA_ERR_INVAL;
    }
    return dlist_insert_after_node(list, list->sent, elem);
}

cdsa_status_t cdsa_dlist_pop_front(cdsa_dlist_t *list, void *out)
{
    if (list == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->sent->next == NULL) {
        return CDSA_ERR_EMPTY;
    }
    dlist_remove_and_free(list, list->sent->next, out);
    return CDSA_OK;
}

/* ── Push / pop back ─────────────────────────────────────────────────────── */

cdsa_status_t cdsa_dlist_push_back(cdsa_dlist_t *list, const void *elem)
{
    if (list == NULL || elem == NULL) {
        return CDSA_ERR_INVAL;
    }
    return dlist_insert_after_node(list, list->tail, elem);
}

cdsa_status_t cdsa_dlist_pop_back(cdsa_dlist_t *list, void *out)
{
    if (list == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->tail == list->sent) {
        return CDSA_ERR_EMPTY;
    }
    dlist_remove_and_free(list, list->tail, out);
    return CDSA_OK;
}

/* ── Peek ────────────────────────────────────────────────────────────────── */

cdsa_status_t cdsa_dlist_peek_front(const cdsa_dlist_t *list, void *out)
{
    if (list == NULL || out == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->sent->next == NULL) {
        return CDSA_ERR_EMPTY;
    }
    memcpy(out, node_data(list->sent->next), list->elem_size);
    return CDSA_OK;
}

cdsa_status_t cdsa_dlist_peek_back(const cdsa_dlist_t *list, void *out)
{
    if (list == NULL || out == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->tail == list->sent) {
        return CDSA_ERR_EMPTY;
    }
    memcpy(out, node_data(list->tail), list->elem_size);
    return CDSA_OK;
}

/* ── Node-based insert ───────────────────────────────────────────────────── */

cdsa_status_t cdsa_dlist_insert_before(cdsa_dlist_t *list,
                                        cdsa_dlist_node_t *node,
                                        const void *elem)
{
    if (list == NULL || node == NULL || elem == NULL) {
        return CDSA_ERR_INVAL;
    }
    /* node->prev is never NULL for a real node (it points to the sentinel or
     * a predecessor real node). */
    return dlist_insert_after_node(list, node->prev, elem);
}

cdsa_status_t cdsa_dlist_insert_after(cdsa_dlist_t *list,
                                       cdsa_dlist_node_t *node,
                                       const void *elem)
{
    if (list == NULL || node == NULL || elem == NULL) {
        return CDSA_ERR_INVAL;
    }
    return dlist_insert_after_node(list, node, elem);
}

/* ── Node-based remove ───────────────────────────────────────────────────── */

cdsa_status_t cdsa_dlist_remove_node(cdsa_dlist_t *list,
                                      cdsa_dlist_node_t *node,
                                      void *out)
{
    if (list == NULL || node == NULL) {
        return CDSA_ERR_INVAL;
    }
    dlist_remove_and_free(list, node, out);
    return CDSA_OK;
}

/* ── Find / remove by key ────────────────────────────────────────────────── */

cdsa_status_t cdsa_dlist_find(const cdsa_dlist_t *list, const void *key,
                               cdsa_dlist_node_t **node_out)
{
    if (list == NULL || key == NULL || node_out == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->cmp_fn == NULL) {
        return CDSA_ERR_INVAL;
    }

    cdsa_dlist_node_t *cur = list->sent->next;
    while (cur != NULL) {
        if (list->cmp_fn(node_data(cur), key) == 0) {
            *node_out = cur;
            return CDSA_OK;
        }
        cur = cur->next;
    }
    return CDSA_ERR_NOTFOUND;
}

cdsa_status_t cdsa_dlist_remove(cdsa_dlist_t *list, const void *key, void *out)
{
    if (list == NULL || key == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (list->cmp_fn == NULL) {
        return CDSA_ERR_INVAL;
    }

    cdsa_dlist_node_t *cur = list->sent->next;
    while (cur != NULL) {
        if (list->cmp_fn(node_data(cur), key) == 0) {
            dlist_remove_and_free(list, cur, out);
            return CDSA_OK;
        }
        cur = cur->next;
    }
    return CDSA_ERR_NOTFOUND;
}

/* ── Splice ──────────────────────────────────────────────────────────────── */

cdsa_status_t cdsa_dlist_splice(cdsa_dlist_t *dest, cdsa_dlist_node_t *pos,
                                 cdsa_dlist_t *src)
{
    if (dest == NULL || src == NULL) {
        return CDSA_ERR_INVAL;
    }
    if (dest == src) {
        return CDSA_ERR_INVAL;
    }
    if (dest->elem_size != src->elem_size) {
        return CDSA_ERR_INVAL;
    }
    if (src->len == 0u) {
        return CDSA_OK;
    }

    cdsa_dlist_node_t *src_first = src->sent->next;
    cdsa_dlist_node_t *src_last  = src->tail;

    /* dest_prev is the node in dest that will precede src_first. */
    cdsa_dlist_node_t *dest_prev = (pos != NULL) ? pos->prev : dest->tail;

    /* Stitch src_first after dest_prev. */
    dest_prev->next    = src_first;
    src_first->prev    = dest_prev;

    /* Stitch src_last before pos (or close the chain at the back). */
    src_last->next = pos;
    if (pos != NULL) {
        pos->prev = src_last;
    } else {
        dest->tail = src_last;
    }

    dest->len += src->len;

    /* Reset src to an empty state. */
    src->sent->next = NULL;
    src->tail       = src->sent;
    src->len        = 0u;

    return CDSA_OK;
}

/* ── Reverse ─────────────────────────────────────────────────────────────── */

void cdsa_dlist_reverse(cdsa_dlist_t *list)
{
    if (list == NULL || list->len < 2u) {
        return;
    }

    cdsa_dlist_node_t *old_first = list->sent->next;
    cdsa_dlist_node_t *old_last  = list->tail;

    /* Invariant: len >= 2 implies sent->next != NULL.  The explicit check
     * keeps static analysers happy without changing runtime behaviour. */
    if (old_first == NULL) {
        return;
    }

    /* For each real node, swap its next and prev pointers. */
    cdsa_dlist_node_t *cur = list->sent->next;
    while (cur != NULL) {
        cdsa_dlist_node_t *tmp = cur->next;
        cur->next = cur->prev;
        cur->prev = tmp;
        cur = tmp;
    }

    /* After the swap, old_last->prev == NULL (was its next = NULL before).
     * Fix up the sentinel link and the terminal NULL. */
    old_last->prev   = list->sent;
    list->sent->next = old_last;
    old_first->next  = NULL;
    list->tail       = old_first;
}

/* ── Accessors ───────────────────────────────────────────────────────────── */

size_t cdsa_dlist_len(const cdsa_dlist_t *list)
{
    return (list != NULL) ? list->len : 0u;
}

cdsa_dlist_node_t *cdsa_dlist_head(const cdsa_dlist_t *list)
{
    return (list != NULL) ? list->sent->next : NULL;
}

cdsa_dlist_node_t *cdsa_dlist_tail(const cdsa_dlist_t *list)
{
    if (list == NULL) {
        return NULL;
    }
    return (list->tail != list->sent) ? list->tail : NULL;
}

cdsa_dlist_node_t *cdsa_dlist_node_next(cdsa_dlist_node_t *node)
{
    return (node != NULL) ? node->next : NULL;
}

/*
 * The sentinel is identified by prev == NULL.  For a user-supplied real node,
 * node->prev is either the sentinel (prev->prev == NULL) or a real predecessor.
 * Return NULL when the predecessor would be the sentinel (node is the first).
 */
cdsa_dlist_node_t *cdsa_dlist_node_prev(cdsa_dlist_node_t *node)
{
    if (node == NULL) {
        return NULL;
    }
    if (node->prev == NULL) {
        return NULL; /* node is the sentinel itself — not reachable via public API */
    }
    if (node->prev->prev == NULL) {
        return NULL; /* node->prev is the sentinel; node is the first real node */
    }
    return node->prev;
}

void *cdsa_dlist_node_data(cdsa_dlist_node_t *node)
{
    return (node != NULL) ? node_data(node) : NULL;
}
