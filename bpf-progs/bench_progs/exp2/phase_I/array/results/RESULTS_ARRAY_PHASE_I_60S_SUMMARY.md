# Phase I Array Map – 60s Measurement Summary (10s Warm-up)

Duration: 60s measured (10s warm-up excluded) | Interval: 0.5s | Map: ARRAY (1024 entries)
Testpoint: tracepoint syscalls:sys_enter_bpfprof

## Throughput Rankings
| Rank | Variant | Calls/sec | Total Calls | % of Fastest | Slowdown % |
|------|---------|-----------|-------------|--------------|------------|
| 1 | array_multiple_lookups | 1,321,581 | 79,294,859 | 100.00% | 0.00% |
| 2 | array_lookup_only | 1,315,250 | 78,915,007 | 99.52% | 0.48% |
| 3 | array_sequential_update | 1,308,140 | 78,488,376 | 98.98% | 1.02% |
| 4 | array_lookup_update | 1,297,692 | 77,861,516 | 98.19% | 1.81% |
| 5 | array_update_only | 1,232,442 | 73,946,506 | 93.26% | 6.74% |
| 6 | array_random_update | 1,181,838 | 70,910,262 | 89.43% | 10.57% |

## Key Observations
1. Multi-Lookup Slight Lead: `array_multiple_lookups` edges out single lookup (+0.48%). The margin is small and likely within run-to-run variance; still notable that four helper calls don't reduce aggregate throughput.
2. Update Cost: Pure update (`array_update_only`) incurs ~6.7% slowdown vs single lookup; introducing randomness increases penalty to ~10.5% total vs fastest, indicating diminished cache locality for key slot.
3. Second Helper Amortization: `array_lookup_update` only ~1.81% slower than lookup-only, suggesting per-invocation overhead dominates while memory stays hot.
4. Sequential vs Lookup+Update: Sequential update variant slightly faster than lookup+update (+0.78% relative), implying deterministic key progression aids branch prediction / cache prefetch.
5. Random Penalty Breakdown: Additional ~5% slowdown beyond update-only attributable to unpredictable key index causing more L1 miss / reduced prefetch efficiency.

## Interpretation of Multi-Lookup Behavior
- Cache Line Packing: Values for keys 0..3 live in a single 64-byte cache line (assuming 8-byte values + metadata alignment), so subsequent lookups after first hit are near-zero incremental memory cost.
- Helper Overhead Overlap: Repeated helper prologs may leverage cached map pointer or benefit from CPU instruction scheduling; front-end overhead amortized across longer straight-line sequence.
- ILP & Pipeline Utilization: More instructions give CPU more room to fill reorder buffers, hiding some fixed latency (e.g., branch, map lookup preamble).
- Noise Consideration: <0.5–1% differences require multi-run statistical validation (StdDev expected in similar microbench on modern CPUs can be ~0.3–0.8%).

## Recommended Next Steps
1. Variance Study: Run each variant N=5–10 times; compute mean, stddev, 95% CI to confirm significance of multi-lookup lead.
2. Locality Stress: Introduce spaced key variant (0,256,512,768) and compare throughput to contiguous multi-lookup to isolate cache line sharing effect.
3. Scaling: Test 8 lookups variant to see if throughput continues near-flat or begins to drop (identify inflection point for helper overhead visibility).
4. Noop Baseline: Add tracepoint-attached program performing minimal operations to quantify fixed dispatch cost.
5. Map Size Sensitivity: Repeat with larger ARRAY sizes (e.g., 16K, 64K) to measure impact of reduced cache residency.
6. JIT Inspection (Deferred): Dump xlated/jited instructions to verify helper call sequences and register reuse.

## Interval Stability (Qualitative)
Intervals (0.5s) are tightly clustered with no major spikes, implying sustained CPU frequency and limited scheduling interference after warm-up.

## Data Provenance
Source files: `results_array_*.txt` under `phase_I/array/results/` generated with 10s warm-up and 60s measurement. Analyzer: `analyze_array_results.py` (Phase I labeling).

---
Generated: $(date -u +%Y-%m-%dT%H:%MZ)
