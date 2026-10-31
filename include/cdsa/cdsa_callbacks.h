#ifndef CDSA_CALLBACKS_H
#define CDSA_CALLBACKS_H

#include <stddef.h>
#include <stdint.h>

#include "cdsa/cdsa_status.h"

/**
 * @file cdsa_callbacks.h
 * @brief Common callback typedefs and built-in default implementations.
 *
 * All generic containers in libcdsa accept user callbacks for comparison,
 * hashing, element destruction, and deep-copying.  This header defines those
 * four function-pointer types and ships ready-to-use defaults for the three
 * most common element kinds: native @c int, NUL-terminated C strings, and raw
 * pointers (identity comparison / address hashing).
 *
 * ### Pointer-to-element convention
 * Every callback receives a @c const @c void* (or @c void*) that points to
 * where the element is stored inside the container — not the element value
 * itself.  For a container of @c int the pointer is a @c int*; for a container
 * of @c char* (strings) the pointer is a @c char**.  This matches the
 * convention used by @c qsort / @c bsearch, so @c cdsa_cmp_int plugs directly
 * into @c qsort.
 */

/* ── Callback typedefs ───────────────────────────────────────────────────── */

/**
 * @brief Element-comparison callback (qsort / bsearch style).
 *
 * @param a  Pointer to the first element.
 * @param b  Pointer to the second element.
 * @return   Negative if *a < *b, 0 if equal, positive if *a > *b.
 * @complexity O(1) for fixed-size types; O(n) for string comparisons.
 */
typedef int (*cdsa_cmp_fn)(const void *a, const void *b);

/**
 * @brief Key-hashing callback.
 *
 * @param key       Pointer to the element that holds the key.
 * @param key_size  Size of the element in bytes.  Implementations that derive
 *                  their own length (e.g. NUL-terminated strings) may ignore
 *                  this.
 * @return          64-bit hash.  The same key must always produce the same
 *                  hash within a single process run.
 * @complexity O(key_size) in general; O(n) for NUL-terminated strings.
 */
typedef uint64_t (*cdsa_hash_fn)(const void *key, size_t key_size);

/**
 * @brief Element-destructor callback.
 *
 * Called when a container releases ownership of an element.  Implementations
 * must free any heap resources owned by the element (e.g. a heap-allocated
 * string pointer stored inside a struct field).  The element storage itself
 * (the memory @p elem points into) belongs to the container and must not be
 * freed by this callback.
 *
 * @param elem  Pointer to the element to destroy.  Must not be NULL.
 * @complexity O(1) for simple types; O(n) when freeing deep structures.
 */
typedef void (*cdsa_free_fn)(void *elem);

/**
 * @brief Deep-copy callback.
 *
 * Copy from @p src into @p dst, allocating heap storage where needed for
 * pointer members.  On failure @p dst must be left in a state that is safe
 * to pass to the matching @c cdsa_free_fn (e.g. NUL pointer members).
 *
 * @param dst        Destination element buffer (at least @p elem_size bytes).
 *                   Must not be NULL.
 * @param src        Source element.  Must not be NULL.
 * @param elem_size  Size of the element in bytes.
 * @return           @c CDSA_OK on success; @c CDSA_ERR_NOMEM if allocation
 *                   fails.
 * @complexity O(elem_size) for flat types; O(n) for heap-owning types.
 */
typedef cdsa_status_t (*cdsa_copy_fn)(void *dst, const void *src,
                                      size_t elem_size);

/* ── int defaults ─────────────────────────────────────────────────────────── */

/**
 * @brief Compare two @c int values.
 *
 * @p a and @p b must each point to an @c int.
 * Safe against overflow (does not subtract).  Plugs directly into @c qsort.
 *
 * @param a  Pointer to first @c int.
 * @param b  Pointer to second @c int.
 * @return   Negative, 0, or positive.
 * @complexity O(1) time, O(1) space.
 */
int cdsa_cmp_int(const void *a, const void *b);

/**
 * @brief Hash an @c int value using a 64-bit mixing function.
 *
 * @p key must point to an @c int; @p key_size is ignored.
 *
 * @param key       Pointer to @c int.
 * @param key_size  Ignored.
 * @return          64-bit hash with good avalanche behaviour.
 * @complexity O(1) time, O(1) space.
 */
uint64_t cdsa_hash_int(const void *key, size_t key_size);

/* ── C-string (char *) defaults ───────────────────────────────────────────── */

/**
 * @brief Compare two NUL-terminated C strings via @c strcmp.
 *
 * @p a and @p b must each point to a @c char* (i.e. pointer-to-pointer-to-char
 * — the element is a @c char*, not the @c char array directly).
 *
 * @param a  Pointer to @c char*.
 * @param b  Pointer to @c char*.
 * @return   Negative, 0, or positive per @c strcmp semantics.
 * @complexity O(n) time where n is the common prefix length.
 */
int cdsa_cmp_str(const void *a, const void *b);

/**
 * @brief FNV-1a 64-bit hash of a NUL-terminated C string.
 *
 * @p key must point to a @c char* (pointer-to-pointer-to-char); @p key_size is
 * ignored.
 *
 * @param key       Pointer to @c char*.
 * @param key_size  Ignored.
 * @return          64-bit FNV-1a hash.
 * @complexity O(n) time where n is the string length.
 */
uint64_t cdsa_hash_str(const void *key, size_t key_size);

/**
 * @brief Free a heap-allocated C string stored inside an element.
 *
 * @p elem must point to a @c char* that was allocated via @c cdsa_malloc() (or
 * the active allocator).  The heap buffer is freed and the pointer is set to
 * NULL.  The element storage itself (@p elem) is not freed.
 *
 * @param elem  Pointer to @c char*.  Must not be NULL.
 * @complexity O(1) time.
 */
void cdsa_free_str(void *elem);

/**
 * @brief Deep-copy a C string element via the active allocator.
 *
 * Allocates a new buffer, copies the NUL-terminated string from *src into it,
 * and stores the new pointer in *dst.  On allocation failure *dst is set to
 * NULL and @c CDSA_ERR_NOMEM is returned; the partially-initialised *dst is
 * safe to pass to @c cdsa_free_str.
 *
 * @param dst        Pointer to @c char* destination.  Must not be NULL.
 * @param src        Pointer to @c char* source.  Must not be NULL.
 * @param elem_size  Ignored.
 * @return           @c CDSA_OK or @c CDSA_ERR_NOMEM.
 * @complexity O(n) time where n is the string length.
 */
cdsa_status_t cdsa_copy_str(void *dst, const void *src, size_t elem_size);

/* ── Pointer-identity defaults ────────────────────────────────────────────── */

/**
 * @brief Compare two pointers by numeric address.
 *
 * @p a and @p b must each point to a @c void* (pointer-to-pointer).
 *
 * @param a  Pointer to @c void*.
 * @param b  Pointer to @c void*.
 * @return   Negative, 0, or positive.
 * @complexity O(1) time, O(1) space.
 */
int cdsa_cmp_ptr(const void *a, const void *b);

/**
 * @brief Hash a pointer by its numeric address.
 *
 * @p key must point to a @c void*; @p key_size is ignored.
 *
 * @param key       Pointer to @c void*.
 * @param key_size  Ignored.
 * @return          64-bit hash.
 * @complexity O(1) time, O(1) space.
 */
uint64_t cdsa_hash_ptr(const void *key, size_t key_size);

/* ── Shared no-op / shallow defaults ──────────────────────────────────────── */

/**
 * @brief No-op destructor for value types (int, float, plain pointers).
 *
 * Use this when elements own no heap resources and need no clean-up.
 *
 * @param elem  Ignored.
 * @complexity O(1) time.
 */
void cdsa_free_noop(void *elem);

/**
 * @brief Shallow copy via @c memcpy.
 *
 * Suitable for flat value types: @c int, @c float, POD structs without
 * heap-allocated members.  Always succeeds.
 *
 * @param dst        Destination buffer (at least @p elem_size bytes).
 * @param src        Source buffer (at least @p elem_size bytes).
 * @param elem_size  Number of bytes to copy.
 * @return           Always @c CDSA_OK.
 * @complexity O(elem_size) time.
 */
cdsa_status_t cdsa_copy_memcpy(void *dst, const void *src, size_t elem_size);

#endif /* CDSA_CALLBACKS_H */
