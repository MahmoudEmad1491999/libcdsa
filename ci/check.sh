#!/usr/bin/env bash
# Full local verification. The agent and CI both run exactly this.
set -euo pipefail
cd "$(dirname "$0")/.."

if [ ! -f CMakeLists.txt ]; then echo "no build system yet — nothing to check"; exit 0; fi
JOBS=$(nproc)

echo "== Debug + ASan/UBSan =="
cmake -S . -B build/asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCDSA_SANITIZE=ON >/dev/null
cmake --build build/asan -j "$JOBS"
ctest --test-dir build/asan --output-on-failure -j "$JOBS"

echo "== Release =="
cmake -S . -B build/rel -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON >/dev/null
cmake --build build/rel -j "$JOBS"
ctest --test-dir build/rel --output-on-failure -j "$JOBS"

if command -v valgrind >/dev/null; then
  echo "== Valgrind =="
  ctest --test-dir build/rel -T memcheck --overwrite MemoryCheckCommandOptions="--leak-check=full --error-exitcode=1" -j "$JOBS" >/dev/null \
    || { cat build/rel/Testing/Temporary/MemoryChecker.*.log 2>/dev/null | head -50; exit 1; }
fi

if command -v cppcheck >/dev/null && [ -d src ]; then
  echo "== cppcheck =="
  cppcheck --quiet --error-exitcode=1 --enable=warning,performance,portability --std=c11 -Iinclude src
fi

if command -v clang-tidy >/dev/null && [ -f .clang-tidy ] && [ -d src ]; then
  echo "== clang-tidy =="
  clang-tidy -p build/rel --quiet src/*.c
fi

if command -v doxygen >/dev/null && [ -f docs/Doxyfile.in ]; then
  echo "== Doxygen docs =="
  cmake --build build/rel --target docs
fi

echo "== Module table =="
python3 scripts/gen_module_table.py --check

echo "ALL CHECKS PASSED"
