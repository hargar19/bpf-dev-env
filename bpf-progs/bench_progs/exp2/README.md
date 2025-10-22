# Experiment 2: Map Access Overhead Measurement

## Goal
Measure the performance overhead of BPF map operations during syscall tracing. This builds on Experiment 1's baseline measurements by adding map access operations.

## Methodology

### Map Types to Test

1. **BPF_MAP_TYPE_ARRAY**
   - Pre-allocated array with fixed size
   - O(1) access by index
   - No locking needed (per-entry)
   - Baseline for fastest map access

2. **BPF_MAP_TYPE_PERCPU_ARRAY**
   - Per-CPU variant of array
   - Eliminates cross-CPU contention
   - Higher memory usage (N_CPU × size)

3. **BPF_MAP_TYPE_HASH**
   - Hash table with dynamic entries
   - Hash computation + bucket lookup
   - Requires locking for concurrent access
   - Memory efficient for sparse keys

4. **BPF_MAP_TYPE_PERCPU_HASH**
   - Per-CPU hash table
   - No locking, but higher memory
   - Each CPU has separate instance

5. **BPF_MAP_TYPE_LRU_HASH**
   - Hash with LRU eviction
   - Additional overhead for LRU list maintenance
   - Bounded memory usage

6. **BPF_MAP_TYPE_LRU_PERCPU_HASH**
   - Per-CPU LRU hash
   - Combines per-CPU isolation with LRU

### Access Patterns

For each map type, test these operations:

#### Pattern A: Single Lookup (Read-Only)
```c
u32 key = 0;
u64 *value = bpf_map_lookup_elem(&map, &key);
if (value) {
    // Just read, don't modify
    u64 v = *value;
}
```

#### Pattern B: Lookup + Update (Read-Write)
```c
u32 key = 0;
u64 *value = bpf_map_lookup_elem(&map, &key);
if (value) {
    __sync_fetch_and_add(value, 1);
}
```

#### Pattern C: Direct Update (Array only)
```c
u32 key = 0;
u64 *value = bpf_map_lookup_elem(&map, &key);
if (value) {
    *value += 1;
}
```

#### Pattern D: Multiple Lookups
```c
// Test cache effects and multiple operations per invocation
for (int i = 0; i < 4; i++) {
    u32 key = i;
    u64 *value = bpf_map_lookup_elem(&map, &key);
    if (value) {
        __sync_fetch_and_add(value, 1);
    }
}
```

### Map Configuration

All maps will be configured with:
- **Max entries**: 4096 (reasonable size, fits in L3 cache)
- **Key type**: `u32` (4 bytes)
- **Value type**: `u64` (8 bytes, supports atomic ops)

Per-CPU maps will naturally scale with number of CPUs.

## Expected Results

### Performance Hierarchy (from fastest to slowest)

1. **No map access** (exp1 baseline) - ~600-800ns per syscall
2. **PERCPU_ARRAY** - minimal overhead, no locks, direct index
3. **ARRAY** - slightly slower due to potential contention
4. **PERCPU_HASH** - hash computation but no locks
5. **HASH** - hash computation + locking
6. **LRU_PERCPU_HASH** - adds LRU maintenance
7. **LRU_HASH** - slowest: hash + locks + LRU

### Key Questions to Answer

1. **What is the base cost of map lookup?** (Array single lookup)
2. **How much does hashing add?** (Array vs Hash comparison)
3. **What is the locking overhead?** (Hash vs PerCPU Hash)
4. **What is the LRU overhead?** (Hash vs LRU Hash)
5. **Does read-only vs read-write matter?** (Pattern A vs B)
6. **Do multiple operations scale linearly?** (Pattern D analysis)

## File Naming Convention

BPF programs: `null_tp_<maptype>_<pattern>.kern.c`

Examples:
- `null_tp_array_lookup.kern.c` - Array with single lookup
- `null_tp_hash_update.kern.c` - Hash with lookup+update
- `null_tp_percpu_array_lookup.kern.c` - Per-CPU array with lookup
- `null_tp_lru_hash_multi.kern.c` - LRU hash with multiple lookups

## Comparison to Exp1

| Aspect | Exp1 | Exp2 |
|--------|------|------|
| **Syscall** | bpfprof (470) | bpfprof (470) |
| **BPF Program** | Empty handler (return 0) | Map access operations |
| **Overhead Source** | Pure tracing mechanism | Tracing + map operations |
| **Variants** | TP, kprobe, fentry | Map types + access patterns |
| **Baseline** | ~600-800ns (from my dummy syscall work) | Exp1 tracepoint result |

## Build and Run

```bash
cd /home/hargar/bpf/bpf-env/profiling/bpf-dev-env/bpf-progs/bench_progs/exp2
make

# Test individual map type
./load.user null_tp_array_lookup.kern.o

# Run all tests with script
./run_all_map_tests.sh
```

## Data Collection

For each map type + access pattern combination:
1. Run benchmark for 30 seconds with 0.5s intervals
2. Record throughput (calls per interval)
3. Calculate overhead vs exp1 baseline:
   - `overhead = (baseline_calls - map_calls) / baseline_calls * 100%`
4. Document variance and stability

## References

- BPF Map Types: https://docs.kernel.org/bpf/maps.html
- Performance considerations paper (reference from user)
- Exp1 baseline results
