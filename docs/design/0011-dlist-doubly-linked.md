# 0011 — Doubly linked list with sentinel head node

## Context
Phase 1 sequences task: doubly linked list with insert_before/after, splice, reverse.

## Decision
Use a **non-circular doubly linked list** with a sentinel head node and an explicit
`tail` pointer in the list struct.  The sentinel's `prev` pointer is permanently
`NULL`; this invariant lets `node_prev()` detect whether a node's predecessor is
the sentinel without storing any extra metadata in real nodes.

Key design choices:
- **Separate sentinel allocation** (no element data) — saves `elem_size` bytes per
  list compared with the slist approach.
- **`insert_before` / `insert_after` as the primitives** — `push_front` and
  `push_back` delegate to the same internal helper, keeping the code path uniform.
- **O(1) `pop_back`** via the `tail` pointer (advantage over `cdsa_slist_t`).
- **O(1) `remove_node`** via `prev` pointers (no predecessor walk needed).
- **O(1) `splice`** — only three pointer pairs are updated regardless of src length.
- **O(n) in-place `reverse`** — walk forward swapping `next`/`prev` on every real
  node, then fix the sentinel link and the terminal `NULL` in two assignments.

## Alternatives considered
* **Circular sentinel** — cleaner reverse, but `node_next` and `node_prev` need the
  list pointer to tell when they've hit the sentinel; breaks the simpler API
  signature inherited from slist.
* **Embedding sentinel in list struct (not heap)** — saves one allocation per list;
  rejected because it changes the create/destroy logic significantly and the saving
  is small.

## Consequences
`node_prev(first_real_node)` returns `NULL` (correct — no predecessor).
The randomized differential test exercises push/pop both ends and reverse in a
2 000-step sequence against a plain C array reference.
