# Array Update Benchmark - Cold Cache (Random Keys)# Figure 4: Array Update Operations (Hot Cache)



This benchmark measures BPF array map **UPDATE** operations with **COLD CACHE** behavior using random keys.This experiment measures BPF array map **UPDATE** performance (using `bpf_map_update_elem`) instead of lookups, with **HOT CACHE** - single hot key stays in L1 cache.



## Key Differences from Other Experiments## Experiment Configuration



### vs. Lookup Operations (array_paper / array_cold)- **Operation**: `bpf_map_update_elem()` - **WRITE** instead of read

- **Operation**: Uses `bpf_map_update_elem()` instead of `bpf_map_lookup_elem()`- **Map Type**: `BPF_MAP_TYPE_ARRAY`

- **Write vs Read**: Updates write data to the map, lookups only read- **Map Size**: 4096 entries

- **Performance Impact**: Updates involve:- **Key Pattern**: Single hot key (key=0) - **HOT CACHE**

  - `memcpy()` to copy value data into map storage- **Value Sizes**: 8B, 64B, 256B, 1KB, 4KB

  - Cache write-backs for dirty cache lines- **Operations per syscall**: 100 map updates

  - Memory barriers for synchronization- **Iterations**: 1024 syscalls per repeat

  - Expected to be 2-10× slower than lookups (grows with value size)- **Repeats**: 100

- **Total operations per variant**: 10,240,000

### vs. Hot Cache Updates (array_update_hot)

- **Key Pattern**: ## Key Differences from Lookup Experiments

  - Hot: Uses constant key (0) → same cache lines reused

  - **Cold: Uses random keys** `(bpf_get_prandom_u32() + i*157) % 16384` → forces cache misses| Aspect | Lookup (Figure 3-a) | Update (Figure 4) |

- **Map Size**: 16K entries (vs 4K hot) → up to 64MB for 4KB values|--------|---------------------|-------------------|

- **Cache Behavior**:| **Operation** | `bpf_map_lookup_elem()` | `bpf_map_update_elem()` |

  - Hot: All accesses hit L1/L2 cache| **Action** | Read pointer to value | Write value to map |

  - **Cold: Random pattern thrashes cache** → frequent DRAM accesses| **Complexity** | Simple pointer return | Copy value data |

- **Expected Slowdown**: 2-5× slower than hot cache (especially for larger values)| **Memory ops** | Read-only | Read + Write |

| **Expected cost** | Lower (just lookup) | Higher (lookup + memcpy) |

## Expected Results

## Expected Results

Cold cache updates should show:

1. **Higher latency than hot cache updates** (cache misses + DRAM access)**Updates should be slower than lookups because:**

2. **Much higher latency than cold cache lookups** (memcpy + cache misses)1. **Memory copy overhead**: Must copy value data into map

3. **Strong value size dependency** (4KB >> 1KB >> 256B >> 64B >> 8B)2. **Cache write-back**: Dirty cache lines need write-back

4. **Largest overhead of all experiments** (combines memcpy AND cache miss costs)3. **Memory barriers**: Synchronization for writes

4. **Value size impact**: Should see clear increase with size

Typical numbers in QEMU VM:   - 8B: ~2-3× slower than lookup

- 8B: ~35-50 ns/op (vs ~26 ns hot update, ~22 ns cold lookup)   - 4KB: ~5-10× slower than lookup (large memcpy)

- 4KB: ~400-600 ns/op (vs ~247 ns hot update, ~56 ns cold lookup)

- Cold/Hot ratio: 1.5-2.5× (combines cache miss + write-back overhead)**From paper (approximate):**

- Lookup 8B: ~80-150 ns

## Build and Run- Update 8B: ~200-300 ns

- Lookup 4KB: ~100-200 ns

```bash- Update 4KB: ~800-1500 ns

make clean && make

sudo ./run_all.sh## Running the Experiment

python3 analyze_baseline.py

python3 plot_results.py```bash

```# Build everything

make clean && make

## BPF Stack Limit Solution

# Run full experiment (as root for CPU pinning and governor)

Large value structures (256B, 1KB, 4KB) cannot fit on the BPF stack (512 byte limit). sudo ./run_all.sh

We use **per-CPU array maps** as value buffers to work around this:

- Each value size has its own buffer map (value_buf_8b, value_buf_64b, etc.)# Analyze results

- BPF program looks up the buffer once per invocationpython3 analyze_baseline.py

- Initializes the buffer with data

- Uses the buffer pointer in all update calls# Generate plots with error bars

- No stack allocation needed!python3 plot_results.py


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
