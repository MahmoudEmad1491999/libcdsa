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

| Module | Description |
|--------|-------------|
| [`cdsa_alloc`](include/cdsa/cdsa_alloc.h) | Process-wide allocator hook and internal allocation wrappers. |
| [`cdsa_callbacks`](include/cdsa/cdsa_callbacks.h) | Common callback typedefs and built-in default implementations. |
| [`cdsa_status`](include/cdsa/cdsa_status.h) | Error codes returned by all fallible cdsa functions. |
| [`cdsa_vec`](include/cdsa/cdsa_vec.h) | Dynamic array (vector) with generic elements and geometric growth. |

## License
MIT
