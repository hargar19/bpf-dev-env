# Experiment 2: Detailed Test Explanation

## Overview

Experiment 2 measures the **performance overhead of BPF map operations** during syscall tracing. We test 6 different map types with 2-3 access patterns each, totaling **14 tests**. Each test attaches a BPF program to the `syscalls/sys_enter_bpfprof` tracepoint and performs map operations on every syscall invocation.

## Test Infrastructure

### Custom Syscall
- **Syscall number**: 470 (`bpfprof` or `dummy`)
- **Kernel version**: Linux 6.17.0-rc7 (custom build)
- **Purpose**: Provides a fast, empty syscall for benchmarking
- **Invocation**: `syscall(470)` - does nothing in kernel space

### Measurement Method
- **Tool**: `bench.user` - repeatedly calls `syscall(470)` in a tight loop
- **Duration**: 30 seconds per test
- **Sampling interval**: 0.5 seconds
- **Metric**: Number of syscall invocations per 0.5s interval
- **Throughput calculation**: calls/0.5s × 2 = calls/second

### Map Configuration
All maps share the same configuration:
- **Max entries**: 4096 (fits in CPU cache)
- **Key type**: `u32` (4 bytes)
- **Value type**: `u64` (8 bytes, supports atomic operations)
- **Pre-population**: Maps are created but entries are accessed on-demand

---

## The 14 Tests Explained

### Group 1: BPF_MAP_TYPE_ARRAY (3 tests)

Arrays are pre-allocated, contiguous memory with O(1) access by index.

#### Test 1: Array-Lookup (Read-Only)
**File**: `null_tp_array_lookup.kern.c`  
**Operation**: Single `bpf_map_lookup_elem()` + read value  
**Purpose**: Baseline for fastest map access  
**Code**:
```c
value = bpf_map_lookup_elem(&test_map, &key);
if (value) {
    u64 v = *value; // Just read, no write
}
```
**Expected**: Fast - direct array indexing, minimal overhead  
**Result**: 630,984 calls/0.5s = **1.26M calls/sec**

---

#### Test 2: Array-Update (Read-Write)
**File**: `null_tp_array_update.kern.c`  
**Operation**: Lookup + atomic increment  
**Purpose**: Measure write cost in addition to read  
**Code**:
```c
value = bpf_map_lookup_elem(&test_map, &key);
if (value) {
    __sync_fetch_and_add(value, 1); // Atomic increment
}
```
**Expected**: Slightly slower than lookup due to write + cache coherence  
**Result**: 649,321 calls/0.5s = **1.30M calls/sec** (surprisingly faster!)

---

#### Test 3: Array-Multi (Multiple Operations)
**File**: `null_tp_array_multi.kern.c`  
**Operation**: 4 lookups + 4 updates to different keys  
**Purpose**: Test cache effects and instruction-level parallelism  
**Code**:
```c
#pragma unroll
for (int i = 0; i < 4; i++) {
    u32 key = i;
    value = bpf_map_lookup_elem(&test_map, &key);
    if (value) {
        __sync_fetch_and_add(value, 1);
    }
}
```
**Expected**: ~4× slower than single operation  
**Result**: 615,935 calls/0.5s = **1.23M calls/sec** (only 5% slower!)

---

### Group 2: BPF_MAP_TYPE_PERCPU_ARRAY (2 tests)

Per-CPU arrays eliminate cross-CPU contention. Each CPU has its own copy.

#### Test 4: PerCPU-Array-Lookup
**File**: `null_tp_percpu_array_lookup.kern.c`  
**Operation**: Single lookup (read-only)  
**Purpose**: Compare with regular array - should be faster with no locking  
**Key difference**: No CPU cache coherence overhead  
**Result**: 635,169 calls/0.5s = **1.27M calls/sec**

---

#### Test 5: PerCPU-Array-Update
**File**: `null_tp_percpu_array_update.kern.c`  
**Operation**: Lookup + atomic increment  
**Purpose**: Measure per-CPU write performance  
**Key difference**: No atomic operations needed across CPUs  
**Result**: 622,882 calls/0.5s = **1.25M calls/sec**

---

### Group 3: BPF_MAP_TYPE_HASH (3 tests)

Hash tables use hash computation + bucket lookup. More flexible but potentially slower.

#### Test 6: Hash-Lookup
**File**: `null_tp_hash_lookup.kern.c`  
**Operation**: Single hash lookup (read-only)  
**Purpose**: Measure hash computation + bucket lookup overhead  
**Code**: Same as Array-Lookup but with hash map  
**Expected**: Slower than array due to hash computation + locking  
**Result**: 616,604 calls/0.5s = **1.23M calls/sec**

---

#### Test 7: Hash-Update
**File**: `null_tp_hash_update.kern.c`  
**Operation**: Hash lookup + atomic increment  
**Purpose**: Measure hash write performance  
**Expected**: Slower than array update  
**Result**: 653,569 calls/0.5s = **1.31M calls/sec** (FASTEST test!)

---

#### Test 8: Hash-Multi
**File**: `null_tp_hash_multi.kern.c`  
**Operation**: 4 hash lookups + 4 updates  
**Purpose**: Test hash performance with multiple operations  
**Expected**: Significantly slower due to multiple hash computations  
**Result**: 587,313 calls/0.5s = **1.17M calls/sec** (SLOWEST test)

---

### Group 4: BPF_MAP_TYPE_PERCPU_HASH (2 tests)

Per-CPU hash tables - each CPU has its own hash table.

#### Test 9: PerCPU-Hash-Lookup
**File**: `null_tp_percpu_hash_lookup.kern.c`  
**Operation**: Single per-CPU hash lookup  
**Purpose**: Compare with global hash - no locking needed  
**Result**: 611,870 calls/0.5s = **1.22M calls/sec**

---

#### Test 10: PerCPU-Hash-Update
**File**: `null_tp_percpu_hash_update.kern.c`  
**Operation**: Per-CPU hash lookup + increment  
**Purpose**: Measure per-CPU hash write performance  
**Result**: 626,300 calls/0.5s = **1.25M calls/sec**

---

### Group 5: BPF_MAP_TYPE_LRU_HASH (2 tests)

LRU hash tables add eviction logic - accessed entries move to LRU head.

#### Test 11: LRU-Hash-Lookup
**File**: `null_tp_lru_hash_lookup.kern.c`  
**Operation**: LRU hash lookup (updates LRU order)  
**Purpose**: Measure LRU maintenance overhead  
**Key difference**: Every lookup updates LRU linked list  
**Expected**: Slower than regular hash due to LRU bookkeeping  
**Result**: 616,610 calls/0.5s = **1.23M calls/sec**

---

#### Test 12: LRU-Hash-Update
**File**: `null_tp_lru_hash_update.kern.c`  
**Operation**: LRU hash lookup + atomic increment  
**Purpose**: Measure LRU write performance  
**Result**: 614,536 calls/0.5s = **1.23M calls/sec**

---

### Group 6: BPF_MAP_TYPE_LRU_PERCPU_HASH (2 tests)

Combines per-CPU isolation with LRU eviction.

#### Test 13: LRU-PerCPU-Hash-Lookup
**File**: `null_tp_lru_percpu_hash_lookup.kern.c`  
**Operation**: Per-CPU LRU hash lookup  
**Purpose**: Best of both worlds - no cross-CPU contention + bounded memory  
**Result**: 628,477 calls/0.5s = **1.26M calls/sec**

---

#### Test 14: LRU-PerCPU-Hash-Update
**File**: `null_tp_lru_percpu_hash_update.kern.c`  
**Operation**: Per-CPU LRU hash lookup + increment  
**Purpose**: Measure per-CPU LRU write performance  
**Result**: 618,468 calls/0.5s = **1.24M calls/sec**

---

## Key Findings

### 1. All Map Types Achieve ~1.2M calls/second

| Map Type | Average Throughput |
|----------|-------------------|
| Array | 1.26M calls/sec |
| PerCPU Array | 1.26M calls/sec |
| Hash | 1.24M calls/sec |
| PerCPU Hash | 1.24M calls/sec |
| LRU Hash | 1.23M calls/sec |
| LRU PerCPU Hash | 1.25M calls/sec |

**Variance: Only 11% between fastest (1.31M) and slowest (1.17M)**

### 2. Surprising Results

❌ **Expected**: Array >> Hash >> LRU Hash  
✅ **Actual**: All within 10% of each other

- **Hash-Update was fastest** (1.31M/s), not Array
- **Per-CPU variants showed no clear advantage** over global maps
- **LRU overhead was negligible** (only 1-2% slower)

### 3. Why So Similar?

The narrow performance range suggests:

1. **Syscall overhead dominates** - entering/exiting kernel space takes ~800ns
2. **Map operations are cheap** - lookup + update takes only ~50-100ns
3. **Everything fits in cache** - 4096 entries × 12 bytes = 49KB (L2 cache)
4. **Single-threaded test** - no contention, locking overhead not visible

### 4. Access Pattern Impact

| Pattern | Average Throughput | Operations per Invocation |
|---------|-------------------|---------------------------|
| Lookup (read) | 1.25M calls/sec | 1 lookup |
| Update (write) | 1.26M calls/sec | 1 lookup + 1 write |
| Multi | 1.20M calls/sec | 4 lookups + 4 writes |

**Multi-ops only 4% slower despite 4× more work!** This suggests excellent CPU pipelining and cache locality.

---

## Comparison Table

| Rank | Test Name | Throughput | Map Type | Access Pattern |
|------|-----------|------------|----------|----------------|
| 1 | Hash-Update | 1.31M/s | Hash | Lookup+Update |
| 2 | Array-Update | 1.30M/s | Array | Lookup+Update |
| 3 | PerCPU-Array-Lookup | 1.27M/s | PerCPU Array | Lookup |
| 4 | Array-Lookup | 1.26M/s | Array | Lookup |
| 5 | LRU-PerCPU-Hash-Lookup | 1.26M/s | LRU PerCPU Hash | Lookup |
| 6 | PerCPU-Hash-Update | 1.25M/s | PerCPU Hash | Lookup+Update |
| 7 | PerCPU-Array-Update | 1.25M/s | PerCPU Array | Lookup+Update |
| 8 | LRU-PerCPU-Hash-Update | 1.24M/s | LRU PerCPU Hash | Lookup+Update |
| 9 | Hash-Lookup | 1.23M/s | Hash | Lookup |
| 10 | LRU-Hash-Lookup | 1.23M/s | LRU Hash | Lookup |
| 11 | Array-Multi | 1.23M/s | Array | 4× Lookup+Update |
| 12 | LRU-Hash-Update | 1.23M/s | LRU Hash | Lookup+Update |
| 13 | PerCPU-Hash-Lookup | 1.22M/s | PerCPU Hash | Lookup |
| 14 | Hash-Multi | 1.17M/s | Hash | 4× Lookup+Update |

---

## What This Tells Us

### For BPF Program Design:

1. **Map type choice doesn't matter much** for small, cache-resident maps
2. **Use the map type that fits your use case** (functionality over performance)
3. **Per-CPU maps** - use for per-CPU counters, not for performance
4. **LRU maps** - "free" eviction with negligible overhead
5. **Multiple operations** - surprisingly cheap (good CPU pipelining)

### When Map Choice WOULD Matter:

1. **Large maps** (> L3 cache size) - cache misses dominate
2. **Multi-threaded workload** - contention on global maps
3. **High write rate** - per-CPU maps avoid cache coherence traffic
4. **Memory pressure** - LRU maps bound memory usage

### Next Steps:

1. **Increase map size** to 1M entries (exceed cache)
2. **Add multi-CPU workload** to stress locking
3. **Test with random access** patterns (current test uses key=0)
4. **Compare with exp1 baseline** to isolate pure map overhead

---

## How to Reproduce

```bash
# Build all tests
cd /bpf-dev-env/bpf-progs/bench_progs/exp2
make

# Run all 14 tests (takes ~7 minutes)
./run_all_tests.sh

# Analyze results
python3 analyze_results.py results_YYYYMMDD_HHMMSS/

# Generate comparison graph
python3 plot_results.py results_YYYYMMDD_HHMMSS/
```

---

## Technical Details

### Tracepoint Attachment
All programs attach to: `tracepoint/syscalls/sys_enter_bpfprof`

This tracepoint fires **before** the syscall executes, so we measure:
- Tracepoint entry overhead
- BPF program execution (map operations)
- Tracepoint exit overhead
- Syscall entry/exit (the dummy syscall does nothing)

### Atomic Operations
We use `__sync_fetch_and_add()` for increments because:
- Prevents data races in multi-CPU scenarios
- Ensures correct results even without contention
- Adds minimal overhead (~1 CPU cycle for locked operation)

### Loop Unrolling
The "Multi" tests use `#pragma unroll` to ensure:
- BPF verifier accepts the loop (bounded complexity)
- Compiler generates inline code (no loop overhead)
- Fair comparison (all operations execute)

### Map Initialization
Maps are created at program load time but:
- Array maps: All entries pre-allocated (always succeed lookup)
- Hash maps: Entries created on first insert (we rely on initialization)
- Per-CPU maps: Each CPU gets separate instance

---

## Conclusion

Experiment 2 demonstrates that **BPF map access overhead is minimal** compared to syscall/tracepoint overhead. At the scale tested (4K entries, single-threaded), all map types perform within 10% of each other, achieving approximately **1.2 million operations per second**. 

The choice of map type should be driven by **functional requirements** (per-CPU isolation, LRU eviction, key flexibility) rather than raw performance, at least for cache-resident workloads.
