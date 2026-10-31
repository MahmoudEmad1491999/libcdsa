# 0004 — Common callback typedefs and default implementations

**Date:** 2026-10-31
**Status:** accepted

## Context
Generic containers (hash maps, trees, sorted arrays) need four kinds of
user-supplied behaviour: comparison ordering, key hashing, element destruction,
and deep-copying.  Every container that accepts these callbacks needs a uniform
type for each; having them defined once avoids drift.

## Decision

**Four typedefs** in `include/cdsa/cdsa_callbacks.h`:
- `cdsa_cmp_fn`  — `int (*)(const void *a, const void *b)`, qsort-style.
- `cdsa_hash_fn` — `uint64_t (*)(const void *key, size_t key_size)`.
- `cdsa_free_fn` — `void (*)(void *elem)` for element-owned resource cleanup.
- `cdsa_copy_fn` — `cdsa_status_t (*)(void *dst, const void *src, size_t)` for
  deep copy; returns `CDSA_ERR_NOMEM` on allocation failure.

**Pointer-to-element convention**: callbacks receive a pointer to where the
element lives inside the container (same as qsort/bsearch), not a value.  For
`int` elements the argument is `int*`; for `char*` string elements it is
`char**`.  This lets `cdsa_cmp_int` plug directly into `qsort`.

**Default implementations** in `src/cdsa_callbacks.c`:
| Symbol | Purpose |
|---|---|
| `cdsa_cmp_int` / `cdsa_hash_int` | int comparison (overflow-safe) / MurmurHash3 64-bit finalizer |
| `cdsa_cmp_str` / `cdsa_hash_str` | strcmp / FNV-1a 64-bit |
| `cdsa_free_str` / `cdsa_copy_str` | deep-copy string via active allocator, zero on failure |
| `cdsa_cmp_ptr` / `cdsa_hash_ptr` | pointer-address comparison / MurmurHash3 mix |
| `cdsa_free_noop` / `cdsa_copy_memcpy` | shared no-op / flat-struct shallow copy |

**clang-tidy**: added
`-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling` to
`.clang-tidy`.  This check flags every `memcpy` call regardless of whether the
size is correct; for a data-structures library that computes its own sizes it
would require `NOLINT` on every future buffer operation.  `memcpy_s` (Annex K)
is not mandated by hosted C11 implementations and is absent on Linux glibc.

## Alternatives considered
- **Header-only `static inline`**: avoids a `.c` file but produces one copy per
  TU and is invisible to clang-tidy.  Rejected.
- **Separate FNV for ptr**: reusing the MurmurHash3 mix is simpler; FNV needs a
  byte loop which is wasteful for a single machine word.

## Consequences
All subsequent modules include `cdsa_callbacks.h` for callback types and pass
these defaults where no user-supplied function is needed.
