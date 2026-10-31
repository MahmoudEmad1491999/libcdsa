# 0005 — Doxygen config and README module table generator

**Date:** 2026-10-31
**Status:** accepted

## Context
Phase 0 requires a `docs` CMake target and a script to keep the README
Modules table in sync with the headers as the library grows.

## Decision

**`docs/Doxyfile.in`**: CMake configures it into `<build>/Doxyfile` via
`configure_file(@ONLY)`, substituting `@PROJECT_VERSION@` and
`@CMAKE_CURRENT_SOURCE_DIR@`.  Output goes to `docs/html/` (HTML) and
`docs/xml/` (machine-readable), both already gitignored.  The `docs` target
is only added when `find_package(Doxygen)` succeeds, so the build stays
valid without Doxygen installed.

**`scripts/gen_module_table.py`**: pure-Python 3, zero dependencies.
Parses each `include/cdsa/*.h` for the `@file @brief` Doxygen comment and
builds a Markdown table.  Three modes:
- (default) print to stdout
- `--update-readme` splice into `## Modules` section in place
- `--check` (used in `ci/check.sh`) exit 1 if the README section is stale

## Alternatives considered
- **Parse Doxygen XML instead of headers**: more robust but requires running
  Doxygen first, creating a dependency loop.  Rejected in favour of direct
  header parsing.
- **Embedded CMake Doxygen helper**: CMake's `doxygen_add_docs()` helper
  (3.9+) hides the Doxyfile.  Rejected because an explicit Doxyfile.in is
  more transparent for future tuning.

## Consequences
Developers who add a new header must run
`python3 scripts/gen_module_table.py --update-readme` before committing;
`ci/check.sh` enforces this.
