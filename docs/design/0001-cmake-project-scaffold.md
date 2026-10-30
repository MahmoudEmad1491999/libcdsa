# 0001 — CMake project scaffold

**Date:** 2026-10-30
**Status:** accepted

## Context
Phase 0 needs a build system before any real module work can begin. The CI
script (`ci/check.sh`) already assumes CMake + Ninja and specific flag names
(`CDSA_SANITIZE`, `CDSA_BUILD_TESTS`), so those options are mandatory.

## Decision
- `CMakeLists.txt` defines two INTERFACE targets (`cdsa_warnings`,
  `cdsa_sanitize`) that every target links against, keeping flag management
  in one place rather than scattered across subdirectories.
- `include(CTest)` (not just `enable_testing()`) is used so that CMake
  generates `DartConfiguration.tcl`, which `ctest -T memcheck` requires to
  locate Valgrind.
- A minimal `src/cdsa_placeholder.c` keeps the static library non-empty;
  empty `STATIC` libraries are rejected by some archivers and linkers.
- `.clang-tidy` uses `WarningsAsErrors: '*'` to match `-Werror` at the
  compiler level.

## Alternatives considered
- **Meson**: rejected — `check.sh` is already written for CMake.
- **Single mega-CMakeLists**: rejected in favour of one
  `add_subdirectory(tests)` per phase for reviewability.

## Consequences
Every subsequent module just calls `cdsa_add_test(test_<module>)` in
`tests/CMakeLists.txt` and links against the `cdsa` target.
