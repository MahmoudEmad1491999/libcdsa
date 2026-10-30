# libcdsa — rules for the coding agent

## Goal
A production-quality, generic data-structures & algorithms library in C11 that a reviewer can trust:
clear APIs, documented complexity, exhaustive tests, zero sanitizer/Valgrind findings, benchmarks with real numbers.

## Layout
```
include/cdsa/<module>.h   public API, Doxygen comments, complexity noted per function
src/<module>.c            implementation
tests/test_<module>.c     one test executable per module (CTest), uses tests/check.h
bench/bench_<module>.c    micro-benchmarks (optional per module), results → docs/bench/
examples/                 small programs using the library
docs/design/              decision records   docs/journal/  (human-only — never edit)
```

## Code rules
- C11, `-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow`. No compiler extensions in public headers.
- Prefix everything `cdsa_`; types `cdsa_<thing>_t`. Opaque structs for containers.
- Generic via `void*` + element size + user callbacks (`cdsa_cmp_fn`, `cdsa_hash_fn`, `cdsa_free_fn`). No macros-as-templates except where a task says so.
- Every allocation goes through `cdsa_alloc.h` hooks (so tests can inject failures). Every function that can fail returns `cdsa_status_t`.
- No global mutable state. No `printf` in library code.
- Each public function documents: purpose, params, return, ownership, time/space complexity.

## Testing rules
- Every new function gets tests: normal cases, edge cases (empty, one element, capacity boundaries), and allocation-failure cases.
- Randomized tests compare against a simple reference implementation with a fixed seed.
- `./ci/check.sh` must pass: Debug+ASan+UBSan build & tests, Release build, Valgrind on tests, clang-tidy, cppcheck.

## Commit style
Conventional Commits, scope = module: `feat(hashmap): add robin-hood probing`.
Never edit `docs/journal/`.
