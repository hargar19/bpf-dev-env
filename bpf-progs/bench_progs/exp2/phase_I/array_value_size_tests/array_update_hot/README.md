# Figure 4: Array Update Operations (Hot Cache)

This experiment measures BPF array map **UPDATE** performance (using `bpf_map_update_elem`) instead of lookups, with **HOT CACHE** - single hot key stays in L1 cache.

## Experiment Configuration

- **Operation**: `bpf_map_update_elem()` - **WRITE** instead of read
- **Map Type**: `BPF_MAP_TYPE_ARRAY`
- **Map Size**: 4096 entries
- **Key Pattern**: Single hot key (key=0) - **HOT CACHE**
- **Value Sizes**: 8B, 64B, 256B, 1KB, 4KB
- **Operations per syscall**: 100 map updates
- **Iterations**: 1024 syscalls per repeat
- **Repeats**: 100
- **Total operations per variant**: 10,240,000

## Key Differences from Lookup Experiments

| Aspect | Lookup (Figure 3-a) | Update (Figure 4) |
|--------|---------------------|-------------------|
| **Operation** | `bpf_map_lookup_elem()` | `bpf_map_update_elem()` |
| **Action** | Read pointer to value | Write value to map |
| **Complexity** | Simple pointer return | Copy value data |
| **Memory ops** | Read-only | Read + Write |
| **Expected cost** | Lower (just lookup) | Higher (lookup + memcpy) |

## Expected Results

**Updates should be slower than lookups because:**
1. **Memory copy overhead**: Must copy value data into map
2. **Cache write-back**: Dirty cache lines need write-back
3. **Memory barriers**: Synchronization for writes
4. **Value size impact**: Should see clear increase with size
   - 8B: ~2-3× slower than lookup
   - 4KB: ~5-10× slower than lookup (large memcpy)

**From paper (approximate):**
- Lookup 8B: ~80-150 ns
- Update 8B: ~200-300 ns
- Lookup 4KB: ~100-200 ns
- Update 4KB: ~800-1500 ns

## Running the Experiment

```bash
# Build everything
make clean && make

# Run full experiment (as root for CPU pinning and governor)
sudo ./run_all.sh

# Analyze results
python3 analyze_baseline.py

# Generate plots with error bars
python3 plot_results.py

# Validate operation counter (in another terminal while loader running)
sudo ./validate_iterations.sh
```

## Comparing with Lookup Performance

After running both experiments:

```bash
# From phase_I directory
cd ..
python3 compare_lookup_update.py  # Compare array_paper vs array_update_hot
```

This will show the **update overhead** compared to lookups for each value size.

## Code Structure

**Key changes from lookup experiment:**

### Baseline (noop)
```c
// Same as lookup - just empty loop overhead
for (int i = 0; i < 100; i++) {
    asm volatile("");
}
```

### Update variants
```c
// Instead of: value = bpf_map_lookup_elem(&map, &key);
// We do:
u32 key = 0;
struct value_Xb value;
// Initialize value data
for (int j = 0; j < SIZE; j++) {
    value.data[j] = 0xdeadbeef;
}
// UPDATE operation
for (int i = 0; i < 100; i++) {
    bpf_map_update_elem(&map, &key, &value, BPF_ANY);
}
```

**Note:** Value initialization is done OUTSIDE the measurement loop to isolate pure update cost.

## What to Expect

1. **Higher latency than lookups**: Updates involve memcpy
2. **Clear value size trend**: Large values take longer to copy
3. **Hot cache benefit**: Key lookup part still fast, only value copy scales
4. **Stable measurements**: Hot key = consistent cache behavior

## Files

- `array_bench.kern.c` - BPF programs with update operations
- `bench_loader.c` - Loads and attaches BPF programs
- `bench_run.c` - Userspace measurement driver
- `run_all.sh` - Automation script
- `analyze_baseline.py` - Statistical analysis
- `plot_results.py` - Visualization with error bars
- `read_counter.c` - Operation counter validation tool

## Next Steps

After completing hot cache updates:
- **Cold cache updates**: `../array_update_cold/` - random keys
- **Compare lookup vs update**: See performance difference
- **Per-CPU arrays**: `../array_percpu/` - CPU-local storage
