# 0003 — Minimal test framework (tests/check.h)

## Context
Every new module needs a consistent, low-overhead way to write tests that
produce machine-readable output for CTest and CI.  The existing tests used raw
`assert()` with a hand-rolled `main()`, giving no per-test pass/fail feedback
and no output for green runs.

## Decision
Implement the framework as a single header `tests/check.h` (no extra .c file,
no external dependency).  It emits **TAP 14** output to stdout so standard TAP
consumers (prove, ctest --output-on-failure) can parse results.

Key design points:
- `static int cdsa_check_failures` — file-scope counter reset before each test.
- Nine `CDSA_CHECK_*` / `CHECK_*` macros; non-fatal (execution continues).
- `cdsa_test_entry_t` + `CDSA_T()` for manual table-driven registration (pure
  C11, no `__attribute__((constructor))` or linker tricks).
- `cdsa_run_suite()` is `static` (one copy per TU = one per test executable).
- Diagnostics go to stderr; TAP lines go to stdout — TAP consumers see a clean
  stream.

## Alternatives considered
- **External framework (Check, Unity, cmocka)**: adds a build dependency and
  complicates the CMake setup for a library that aims for zero deps.
- **`__attribute__((constructor))` auto-registration**: non-standard; banned in
  public headers by CLAUDE.md.

## Consequences
- Each test file includes `check.h` and calls `cdsa_run_suite()` from `main()`.
- clang-tidy's `misc-definitions-in-headers` check does not apply here because
  the tidy run covers only `src/*.c`, not `tests/`.
- A deliberate failure in the self-test (`test_failure_detection`) emits one
  stderr diagnostic line; CI output notes it is expected.
