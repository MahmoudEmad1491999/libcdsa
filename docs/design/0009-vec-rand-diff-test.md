# 0009 — Vector: randomized differential test

## Context
The existing `test_vec` suite tests each vector operation in isolation with
hand-crafted inputs.  That leaves multi-operation sequences untested: a bug
that only manifests after a grow-then-erase-then-reverse sequence would slip
through.

## Decision
Add `test_rand_diff` inside `tests/test_vec.c`.  It drives a plain C array
(the reference) and a `cdsa_vec_t` in lockstep through 2 000 randomly chosen
operations (push, pop, insert, erase, set, swap, reverse), seeded with 42 so
failures are reproducible.  After each operation, lengths and all elements are
compared.  The reference implementation uses `memmove`-based helpers that are
too simple to harbour the same bugs as the vector.

## Alternatives
- Separate file `test_vec_rand.c`: rejected; CLAUDE.md says one executable per
  module, so keeping everything in `test_vec.c` is consistent.
- Property-based library (e.g., theft): rejected; no external test dependency
  and 200 lines of LCG + reference code is easier to audit.

## Consequences
- Adds ~175 lines to `test_vec.c`, well within the 400-line budget.
- Seed 42 makes failures deterministic and bisectable.
- Max 128 elements keeps the test under 20 ms even under ASan/Valgrind.
