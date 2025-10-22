# Experiment 2 Results Summary

Generated: Wed Oct 22 05:35:06 UTC 2025

## Test Configuration
- Duration: 30s per test
- Interval: 0.5s
- Total tests: 14

## Results

| Test Name | Avg Calls/Interval | Result File |
|-----------|-------------------|-------------|
| Array-Lookup | 630984 | Array-Lookup.txt |
| Array-Update | 649321 | Array-Update.txt |
| Array-Multi | 615935 | Array-Multi.txt |
| PerCPU-Array-Lookup | 635169 | PerCPU-Array-Lookup.txt |
| PerCPU-Array-Update | 622882 | PerCPU-Array-Update.txt |
| Hash-Lookup | 616604 | Hash-Lookup.txt |
| Hash-Update | 653569 | Hash-Update.txt |
| Hash-Multi | 587313 | Hash-Multi.txt |
| PerCPU-Hash-Lookup | 611870 | PerCPU-Hash-Lookup.txt |
| PerCPU-Hash-Update | 626300 | PerCPU-Hash-Update.txt |
| LRU-Hash-Lookup | 616610 | LRU-Hash-Lookup.txt |
| LRU-Hash-Update | 614536 | LRU-Hash-Update.txt |
| LRU-PerCPU-Hash-Lookup | 628477 | LRU-PerCPU-Hash-Lookup.txt |
| LRU-PerCPU-Hash-Update | 618468 | LRU-PerCPU-Hash-Update.txt |

## Analysis

### Throughput Overview

**Test Interval**: 0.5 seconds  
**Average Throughput Range**: 587,313 - 653,569 calls per 0.5s interval

**Converting to calls/second**:
- **Highest**: 653,569 calls/0.5s = **~1.31 million calls/second** (Hash-Update)
- **Lowest**: 587,313 calls/0.5s = **~1.17 million calls/second** (Hash-Multi)
- **Overall Average**: 622,038 calls/0.5s = **~1.24 million calls/second**

### Performance by Map Type

| Map Type | Avg Calls/0.5s | Throughput (calls/sec) |
|----------|----------------|------------------------|
| **Array** | 632,080 | **~1.26M/s** |
| **PerCPU Array** | 629,026 | **~1.26M/s** |
| **Hash** | 619,162 | **~1.24M/s** |
| **PerCPU Hash** | 619,085 | **~1.24M/s** |
| **LRU Hash** | 615,573 | **~1.23M/s** |
| **LRU PerCPU Hash** | 623,473 | **~1.25M/s** |

### Performance by Access Pattern

| Access Pattern | Avg Calls/0.5s | Throughput (calls/sec) |
|----------------|----------------|------------------------|
| **Lookup** (read-only) | 623,286 | **~1.25M/s** |
| **Update** (read+write) | 630,816 | **~1.26M/s** |
| **Multi** (4 ops) | 601,624 | **~1.20M/s** |

### Top 5 Performers

1. **Hash-Update**: 653,569 calls/0.5s = **1.31M calls/sec**
2. **Array-Update**: 649,321 calls/0.5s = **1.30M calls/sec**
3. **PerCPU-Array-Lookup**: 635,169 calls/0.5s = **1.27M calls/sec**
4. **Array-Lookup**: 630,984 calls/0.5s = **1.26M calls/sec**
5. **LRU-PerCPU-Hash-Lookup**: 628,477 calls/0.5s = **1.26M calls/sec**

### Key Findings

1. **All map types achieve ~1.2M calls/second throughput**
   - Only 11% variance between fastest (1.31M/s) and slowest (1.17M/s)
   - Array maps: 1.26M/s average
   - Hash maps: 1.24M/s average
   - LRU variants: 1.24M/s average

2. **Access pattern has minimal impact**
   - Lookup (read-only): 1.25M/s
   - Update (read+write): 1.26M/s  
   - Multi (4 operations): 1.20M/s (only 5% slower)

3. **Per-CPU variants show no clear advantage**
   - PerCPU Array: 1.26M/s vs Array: 1.26M/s (identical)
   - PerCPU Hash: 1.24M/s vs Hash: 1.24M/s (identical)
   - At this scale, locking overhead is negligible

4. **Hash maps are competitive with arrays**
   - Hash-Update achieves highest throughput (1.31M/s)
   - Suggests hash computation is cheap relative to syscall overhead
   - Array advantage (direct indexing) not evident at this scale

### Interpretation

The results show that **BPF map access overhead is minimal** when:
- Map size is small (4096 entries, fits in cache)
- Single-threaded workload (no contention)
- Simple access patterns (1-4 operations per syscall)

The dominant cost is the **syscall/tracepoint overhead**, not map operations. All map types can sustain **~1.2 million tracepoint invocations per second** with map access included.

### Recommendations

1. **Test with larger maps**: Increase map size to 1M entries to exceed CPU cache and observe real map access costs
2. **Add multi-CPU contention**: Run multiple threads/processes to stress lock contention on shared maps
3. **Vary access patterns**: Test random vs sequential access, different hit rates
4. **Use BPF_STATS**: Run with `load_stats_enabled.user` to get per-program execution time from kernel

### Summary

**Experiment 2 demonstrates that all 6 BPF map types achieve approximately 1.2 million syscall+tracepoint+map-access operations per second**, with less than 11% variance across different map types and access patterns. At this scale (4K entries, single-threaded), map choice has negligible performance impact.
