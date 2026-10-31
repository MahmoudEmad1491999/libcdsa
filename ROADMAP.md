# Roadmap

One task ≈ one day. The agent takes the first unchecked item.

## Phase 0 — Foundations
- [x] CMake project (C11, warnings-as-errors, CDSA_SANITIZE option), `.clang-format`, `.clang-tidy`, `.gitignore`
- [x] `cdsa_status_t` error codes + `cdsa_strerror`, `cdsa_alloc.h` allocator hooks with a failing-allocator test helper
- [ ] Minimal test framework `tests/check.h` (assert macros, test registration, TAP-like output) + a self-test
- [ ] Common callbacks header: `cdsa_cmp_fn`, `cdsa_hash_fn`, `cdsa_free_fn`, `cdsa_copy_fn`, plus default int/str/ptr helpers
- [ ] Doxygen config + `docs` CMake target; README module table generator script

## Phase 1 — Sequences
- [ ] Dynamic array (vector): create/destroy/push/pop/get/set, geometric growth
- [ ] Vector: insert/erase at index, reserve, shrink_to_fit, clear
- [ ] Vector: iteration API, foreach macro, swap, reverse
- [ ] Vector: randomized differential test against a plain C array reference
- [ ] Singly linked list with sentinel: push/pop front/back, find, remove
- [ ] Doubly linked list: insert before/after, splice, reverse
- [ ] Linked list: merge sort on lists, stable, tests with duplicates
- [ ] Intrusive list (Linux-kernel style `container_of`) + example
- [ ] Circular ring buffer (power-of-two capacity): push/pop/peek, overwrite mode
- [ ] Lock-free SPSC ring buffer with C11 atomics + 2-thread stress test
- [ ] Deque on a segmented block map: push/pop both ends, random access
- [ ] Stack and queue adapters over vector/deque
- [ ] Benchmarks: vector vs list vs deque for push/iterate/random access → docs/bench/sequences.md

## Phase 2 — Hashing
- [ ] Hash functions: FNV-1a, SipHash-2-4 (keyed), xxHash-style 64-bit; test vectors
- [ ] Hash map with separate chaining: put/get/remove/len, load-factor resize
- [ ] Hash map: iteration, rehash correctness under deletes, randomized differential test
- [ ] Open-addressing hash map with linear probing + tombstones
- [ ] Robin Hood hashing with backward-shift deletion
- [ ] Hash set built on the open-addressing map
- [ ] Hash map benchmark: chaining vs linear vs robin hood at load 0.5/0.75/0.9
- [ ] Bloom filter with configurable false-positive rate + measured FPR test
- [ ] Count-min sketch + accuracy test
- [ ] LRU cache (hash map + intrusive list), O(1) get/put

## Phase 3 — Trees
- [ ] Binary search tree: insert/find/delete, in/pre/post-order iterators
- [ ] AVL tree: rotations, insert, invariant checker used in tests
- [ ] AVL tree: delete with rebalancing, randomized test vs sorted vector
- [ ] Red-black tree: insert with fix-up, invariant checker
- [ ] Red-black tree: delete with fix-up, randomized test
- [ ] Ordered map API on the RB tree: lower_bound/upper_bound/range iteration
- [ ] Treap with split/merge
- [ ] Skip list with randomized levels, ordered iteration
- [ ] B-tree (order t): search/insert
- [ ] B-tree: delete with merge/borrow, invariant checker
- [ ] Trie for strings: insert/find/prefix iteration/delete
- [ ] Radix (compressed) trie
- [ ] Segment tree with lazy propagation (range add, range sum)
- [ ] Fenwick tree (BIT), 1D and 2D
- [ ] Interval tree (augmented RB): overlap queries
- [ ] Ordered-structure benchmark: AVL vs RB vs skip list vs B-tree → docs/bench/trees.md

## Phase 4 — Heaps & priority
- [ ] Binary heap (min/max via comparator): push/pop/peek/heapify
- [ ] Indexed priority queue with decrease-key
- [ ] d-ary heap and benchmark vs binary heap
- [ ] Pairing heap
- [ ] Fibonacci heap (insert/extract-min/decrease-key) + amortized-behaviour test
- [ ] Union-Find with path compression + union by rank

## Phase 5 — Sorting & searching
- [ ] Insertion, selection, bubble sorts (reference implementations for tests)
- [ ] Merge sort (top-down and bottom-up), stable
- [ ] Quicksort with median-of-three + insertion cutoff
- [ ] Introsort (quicksort → heapsort fallback), qsort-compatible signature
- [ ] Heap sort
- [ ] Counting sort and LSD radix sort for integers
- [ ] Timsort-lite (run detection + galloping merge)
- [ ] Binary search variants: lower/upper bound, exponential search
- [ ] Quickselect and median-of-medians nth_element
- [ ] Sorting benchmark on random/sorted/reversed/few-unique inputs vs libc qsort → docs/bench/sorting.md

## Phase 6 — Graphs
- [ ] Graph representation: adjacency list (directed/undirected, weighted) + edge list import
- [ ] BFS and DFS (iterative), with visitor callbacks
- [ ] Topological sort (Kahn + DFS), cycle detection
- [ ] Connected components; Tarjan SCC
- [ ] Dijkstra with the indexed heap
- [ ] Bellman-Ford with negative-cycle detection
- [ ] Floyd-Warshall + path reconstruction
- [ ] A* on a grid graph with Manhattan heuristic + example maze solver
- [ ] Kruskal and Prim MST
- [ ] Bridges and articulation points
- [ ] Max flow: Edmonds-Karp
- [ ] Max flow: Dinic; compare against Edmonds-Karp
- [ ] Bipartite matching (Hopcroft-Karp)
- [ ] Graph algorithms benchmark on generated sparse/dense graphs

## Phase 7 — Strings
- [ ] Dynamic string type (`cdsa_str`): append/format/slice, always NUL-terminated
- [ ] KMP search
- [ ] Rabin-Karp with rolling hash; multi-pattern
- [ ] Z-algorithm
- [ ] Boyer-Moore-Horspool
- [ ] Aho-Corasick automaton
- [ ] Suffix array (prefix doubling) + LCP (Kasai)
- [ ] Edit distance (Levenshtein) with O(min(n,m)) memory
- [ ] Longest common subsequence with reconstruction

## Phase 8 — Algorithms toolbox
- [ ] Memory pool allocator (fixed-size blocks) plugged into the allocator hooks
- [ ] Arena allocator + benchmark of containers on arena vs malloc
- [ ] Bitset with popcount/find-first-set, iterator
- [ ] Sparse set
- [ ] Knapsack 0/1 and unbounded DP
- [ ] Matrix chain / interval DP example
- [ ] Convex hull (Andrew's monotone chain)
- [ ] Reservoir sampling and Fisher-Yates shuffle with a seeded PRNG (xoshiro256**)
- [ ] Big integer: add/sub/mul (schoolbook) + Karatsuba

## Phase 9 — Hardening & polish
- [ ] libFuzzer harness for hash map (build option CDSA_FUZZ) + short fuzz run in check.sh
- [ ] Fuzz harnesses for RB tree and B-tree
- [ ] Fuzz harness for string algorithms (compare KMP/Z/BMH results)
- [ ] Code coverage target (gcov/lcov), report summary in README
- [ ] Thread-safety notes per module + a mutex-wrapped concurrent hash map
- [ ] `install` target, pkg-config file, CMake package config, example consumer project
- [ ] API reference generated by Doxygen published via GitHub Pages workflow
- [ ] README: module table with complexities, benchmark charts, usage examples
- [ ] Examples: word-frequency counter, LRU-cached file reader, shortest-path CLI
- [ ] Release v1.0.0: CHANGELOG generated from conventional commits, tag notes
