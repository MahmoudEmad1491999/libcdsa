# 0007 — Vector: insert/erase/reserve/shrink_to_fit/clear

## Context
Phase 1 continuation.  The basic push/pop/get/set API (0006) left out the
five operations needed to use the vector as a general-purpose sequence:
positional insert/erase, up-front capacity reservation, memory trimming, and
bulk removal.

## Decisions

**insert/erase use memmove, not a manual loop.**
`memmove` handles overlapping source/destination correctly and benefits from
compiler and libc optimisations.  A manual element-wise loop would be
equivalent but slower and harder to review.

**reserve allocates exactly min_cap, not geometrically.**
The caller passes an explicit count because they already know the required
size.  Applying a growth factor would waste memory against the caller's
intent.  Push continues to use geometric growth independently.

**shrink_to_fit frees the buffer when len == 0.**
Calling `cdsa_realloc(ptr, 0)` has implementation-defined behaviour in C11.
Freeing explicitly is well-defined and leaves the vector in the same state as
a freshly created one, allowing push to work normally afterwards.

**shrink_to_fit returns CDSA_ERR_NOMEM on failure (vector unchanged).**
A hint-only shrink (like C++ `shrink_to_fit`) would silently succeed but waste
memory; returning an error lets callers handle constrained environments.

**clear is void — it cannot fail.**
Clear only calls free_fn per element and resets len; it performs no allocation.
Matching `cdsa_vec_destroy`'s signature keeps the API consistent for
operations that cannot fail.

## Alternatives considered
- Using `realloc(ptr, 0)` in shrink_to_fit: undefined behaviour in C11; rejected.
- Geometric growth in reserve: wastes memory when the caller already knows the
  exact requirement; rejected.

## Consequences
Iteration, swap, and reverse (next roadmap item) can build on these primitives.
