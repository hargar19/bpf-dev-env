We’re seeing two stacked slowdowns:

1. Update-only vs Lookup-only (~8% slower)
2. Random update vs Sequential/fixed update (additional ~3.5–4% slower)

Here’s what drives each.

----------------------------------------
1. Why an update costs more than a lookup
For an Array map, both operations begin by computing the element address (bounds check, pointer math). After that:

Lookup path (bpf_map_lookup_elem):
- Reads the value (a single 8‑byte load here).
- Returns a pointer. No modification, so the cache line stays in Shared state (MESI S) or transitions from Exclusive to Shared only if other cores touch it later.
- Minimal memory ordering requirements. Typically just a load; kernel helper prolog/epilog overhead.

Update path (bpf_map_update_elem):
- Still needs the element address (same as lookup).
- Copies caller’s new value into map memory (a store).
- Store can trigger:
  - Write allocate if the line isn’t already in the L1 in a writable (Modified) state.
  - Coherence messages to move line ownership from Shared to Modified.
  - Potential partial line forwarding penalties if the line wasn’t present (less common for frequently reused keys).
- May include checksum/metadata or argument validation logic in the helper, plus error handling branches (even if they predict well).
- Requires ordering sufficient to ensure visibility of the written value before helper returns (may imply a compiler barrier; full memory barrier is usually not needed for array updates, but there’s still some serialization around helper exit).

Micro-architectural cost differences:
- A store with write-allocate can add ~10–30 cycles if it misses in L1 (pull line from L2), compared to a load hitting L1 at ~4 cycles.
- Store buffer pressure: repeated stores can occasionally stall if the buffer fills (less likely here but contributes in aggregate).
- The update helper might do a memcpy() for value size (even 8 bytes), which the compiler can expand to a couple of instructions (load immediate + store), still more than a single read.

Result: Even on hot lines, each invocation adds a few extra instructions plus the coherence overhead for moving a line into Modified state—accumulating to ~8% throughput drop versus a lookup-only path dominated by load latency and fixed syscall + BPF prolog.

----------------------------------------
2. Why random updates are slower than sequential/fixed updates (extra ~3.5–4%)
Sequential/fixed key patterns keep touching the same cache line (key 0) or a small rotating set that quickly stays resident in L1/L2 and remains in Modified state between stores. This minimizes:

- Cache line replacements (high locality).
- TLB pressure (same page repeatedly).
- Branch mispredicts (address calculation predictable).
- Store write-allocate misses (line already hot).

Random key selection spreads writes across many distinct indices (0..1023):
- Working set expands to many lines (each array value 8 bytes; 1024 entries ≈ 8 KB, fits in L1 but randomization can still cause eviction of the “next” line if other code runs).
- More transitions from Shared to Modified across different lines—coherence overhead multiplies.
- Higher chance of L1 misses (line cold) requiring fetch from L2/L3 before store.
- Possible TLB misses if the access pattern isn’t fully contained in a single page (depends on page layout; 8 KB fits in two 4K pages, so mild TLB effect, but random distribution might touch both pages evenly).
- Less effective hardware prefetch (prefetchers rely on stride or streaming patterns; random lacks this).