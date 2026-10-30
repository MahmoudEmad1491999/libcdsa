# libcdsa

Generic data structures and algorithms in portable C11, built to be trusted: every module is
tested with AddressSanitizer, UndefinedBehaviorSanitizer and Valgrind, fuzzed, and benchmarked.

> 🚧 Built incrementally. See [ROADMAP.md](ROADMAP.md) for progress and [docs/journal](docs/journal) for the development log.

## Build & test
```sh
./ci/check.sh            # full verification (sanitizers, valgrind, static analysis)
cmake -S . -B build && cmake --build build && ctest --test-dir build
```

## Modules
_Filled in as modules land._

## License
MIT
