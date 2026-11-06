# 0008 — Vector: iteration API, foreach macro, swap, reverse

## Context
Phase 1 continuation.  The insert/erase/reserve API (0007) left out element
traversal and in-place reordering.  Callers need a way to visit every element
without manual index management, and a pair of building-block operations
(swap, reverse) that sort and partition algorithms will rely on.

## Decisions

**`cdsa_vec_data` exposes the raw buffer pointer.**
The vector's opaque struct prevents callers from computing pointers themselves.
A direct buffer accessor allows O(1) random access for algorithms (qsort,
bsearch, SIMD loops) without the per-call overhead of `cdsa_vec_get`.
Invalidation on reallocation is documented and consistent with C++ `data()`.

**`CDSA_VEC_FOREACH` uses `cdsa_vec_get` internally, not raw arithmetic.**
Using `cdsa_vec_get` in the macro condition keeps the boundary between public
API and internal representation sharp; the macro does not need to know the
element size.  The `__LINE__`-suffixed counter variable avoids `-Wshadow`
diagnostics when the macro is used in nested scopes on different lines.

**swap and reverse allocate one temporary buffer of elem_size bytes.**
The only allocation-free alternative is XOR-swap (bit-wise for arbitrary
bytes), which has undefined behaviour for objects with padding and fails for
overlapping ranges.  A single malloc per call is simpler and safe.  On
allocation failure both operations leave the vector completely unchanged,
consistent with the rest of the API's atomicity convention.

**reverse is not expressed as n/2 calls to swap.**
Doing so would require n/2 mallocs.  One malloc + an in-place two-pointer
loop is O(n) with O(elem_size) extra space and a single failure point.

## Alternatives considered
- `cdsa_vec_begin`/`cdsa_vec_end` iterator struct: extra type, extra API
  surface, and no benefit over index + `cdsa_vec_data` for a contiguous array.
- Stack-allocated temp in swap/reverse: requires VLAs (non-standard under
  `-Wpedantic`) or a fixed-size buffer with a size cap; rejected.

## Consequences
Sorting and partition algorithms can build on `cdsa_vec_data`, `cdsa_vec_swap`,
and `cdsa_vec_reverse` without touching opaque internals.
