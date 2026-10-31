# 0002 — cdsa_status_t error codes and cdsa_alloc.h allocator hooks

**Date:** 2026-10-31
**Status:** accepted

## Context
Every fallible function needs a uniform return type for callers to check, and
every allocation must be interceptable so tests can inject failure without
replacing the C library.

## Decision

**Error codes** (`cdsa_status.h`): an `enum cdsa_status` with `CDSA_OK = 0`
and negative error values.  Negative-means-error follows the POSIX convention
and lets callers write `if (rc != CDSA_OK)` without caring about exact values.
`cdsa_strerror` uses a `switch` with an explicit `default:` to handle unknown
future values gracefully.

**Allocator hooks** (`cdsa_alloc.h`): a process-wide `cdsa_allocator_t` struct
(three function pointers + an opaque `ctx`) replaces all direct `malloc` /
`realloc` / `free` calls inside the library.  The active hook is stored in a
`static` file-scope variable in `cdsa_alloc.c`.

**Fail counter** (`cdsa_alloc_fail_after`): a second `static size_t` counts
down, causing `cdsa_malloc` and `cdsa_realloc` to return NULL while non-zero.
This deliberately violates the "no global mutable state" rule: the fail counter
*is* global mutable state, but it is the minimal mechanism required by the task
and is documented as single-threaded test-only infrastructure.

## Alternatives considered
- **Per-container allocator**: each container holds a `cdsa_allocator_t *`.
  More flexible, but adds a pointer to every container and complicates every
  API.  Deferred — a Phase 8 arena task can revisit this.
- **Thread-local fail counter**: `_Thread_local` would isolate concurrent tests
  but adds complexity before the library has any threading at all.

## Consequences
All subsequent modules call `cdsa_malloc` / `cdsa_realloc` / `cdsa_free`
instead of the C library directly.  Tests gain `cdsa_alloc_fail_after(n)` to
exercise every allocation-failure path deterministically.
