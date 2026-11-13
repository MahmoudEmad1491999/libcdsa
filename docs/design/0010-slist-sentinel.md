# 0010 — Singly linked list with sentinel head node

## Context
Phase 1 sequences task: singly linked list with push/pop front/back, find, remove.

## Decision
Use a **sentinel head node** (a dummy node whose data area is never read) so
that push, pop, and remove never need to special-case an empty list — `prev`
can always be initialised to the sentinel. A **tail pointer** (initially
pointing to the sentinel) gives O(1) push_back; pop_back stays O(n) since
there is no predecessor pointer.

Both sentinel and real nodes are allocated with the same helper
(`sizeof(node) + elem_size`), keeping a single allocation path. The sentinel
wastes `elem_size` bytes of heap per list; this is acceptable for simplicity.

## Alternatives considered
* **No sentinel** — saves one allocation but adds NULL checks everywhere.
* **Doubly linked list** — O(1) pop_back, but that is the next roadmap task.
* **Allocate sentinel without data** — saves `elem_size` bytes but requires two
  allocation paths; not worth the complexity for a singly-linked list.

## Consequences
pop_back is O(n) by design. The public iteration API
(`cdsa_slist_head`, `cdsa_slist_node_next`, `cdsa_slist_node_data`,
`CDSA_SLIST_FOREACH`) exposes stable node pointers, which are used directly
in the randomized differential test.
