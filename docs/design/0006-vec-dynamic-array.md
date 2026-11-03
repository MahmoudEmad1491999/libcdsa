# 0006 — Dynamic array (vector): core API

## Context
Phase 1 needs a generic dynamic array as the foundational sequence container.
The vector must support arbitrary element types, plug into the allocator hooks,
and guarantee zero sanitizer or Valgrind findings.

## Decisions

**Growth factor 2, initial capacity 4.**
Factor-2 keeps amortised O(1) push cost with a simple bit-shift and the
overflow check `cap > SIZE_MAX / 2` is trivially correct.  Initial cap 4
avoids multiple reallocations for short-lived vectors while wasting little
memory (16 bytes for int).

**Ownership on pop: out-buffer vs free_fn.**
`cdsa_vec_pop(v, out)` transfers the element to `out` when `out != NULL`
(no free_fn call), and calls free_fn when `out == NULL` (discard mode).
This avoids copying just to immediately free when the caller does not need
the value.

**set calls free_fn on the old element.**
The vector owns the element bytes; overwriting without cleanup would leak
resources for types like heap-allocated strings.

**No buffer allocated at create-time.**
`cdsa_vec_create` allocates only the struct, deferring the data buffer to the
first push.  This keeps empty vectors cheap and makes `cdsa_alloc_fail_after`
injection in tests easier to reason about.

## Alternatives considered
- Growth factor 1.5: reduces peak memory but complicates the overflow check
  and offers no measurable benefit at this stage.
- Returning a direct `void *` from get instead of writing through `void **`:
  simpler call site, but cannot return a distinct error code.

## Consequences
insert/erase, reserve, and shrink_to_fit are deferred to the next roadmap item.
