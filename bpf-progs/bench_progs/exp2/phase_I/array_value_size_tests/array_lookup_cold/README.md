# Figure 3-b: Cold Cache Array Lookups# Paper-Style Array Map Benchmark



This experiment measures BPF array map lookup performance with **COLD CACHE** - using random keys across the full keyspace (4096 entries) to cause cache misses.Implementation of the methodology from "Understanding Performance of eBPF Maps" (ACM SIGCOMM 2024).



## Experiment Configuration## Directory Structure



- **Map Type**: `BPF_MAP_TYPE_ARRAY````

- **Map Size**: 4096 entriesarray_paper/

- **Key Pattern**: Random keys (`bpf_get_prandom_u32() % 4096`) - **COLD CACHE**├── array_bench.kern.c    # BPF programs (noop, lookup, update)

- **Value Sizes**: 8B, 64B, 256B, 1KB, 4KB├── bench_loader.c        # Loads and attaches BPF program

- **Operations per syscall**: 100 map lookups├── bench_run.c           # Userspace benchmark runner

- **Iterations**: 1024 syscalls per repeat├── Makefile              # Build system

- **Repeats**: 100├── test_step1.sh         # Test script for current step

- **Total operations per variant**: 10,240,000└── README.md             # This file

```

## Differences from Hot Cache (Figure 3-a)

## Implementation Steps

| Aspect | Hot Cache (3-a) | Cold Cache (3-b) |

|--------|----------------|------------------|### ✅ Step 1: Basic BPF Lookup Program

| Key Pattern | Single hot key (key=0) | Random keys (0-4095) |- **Status**: COMPLETE

| Cache Behavior | All accesses hit L1 cache | Frequent cache misses |- **Files**: `array_bench.kern.c`, `bench_loader.c`, `bench_run.c`

| Expected Latency | ~10-20 ns/op | ~50-200+ ns/op |- **What**: Single lookup operation, 4096-entry array, 8-byte values

| Value Size Effect | Minimal | Significant (more cache lines) |

| Baseline | Empty loop | Empty loop + random generation |### ✅ Step 4: Noop Baseline

- **Status**: COMPLETE

## Expected Results- **Files**: Same as Step 1

- **What**: Empty BPF program for measuring trampoline overhead

**Cold cache should show:**

1. **Higher latency** than hot cache (3-10× slower)### ✅ Step 5: Results & Analysis

2. **Clear increase with value size** due to cache line effects:- **Status**: COMPLETE

   - 8B: ~1 cache line (64 bytes)- **Files**: `run_comparison.sh`, `analyze_results.py`

   - 64B: ~1 cache line- **What**: Automated test runner, results saved to `results/`, analysis script for host VM

   - 256B: ~4 cache lines- **Test**: `sudo ./run_comparison.sh`

   - 1KB: ~16 cache lines

   - 4KB: ~64 cache lines### Step 6-10: TODO

3. **Higher variance** (larger std dev) due to cache behavior variabilitySee todo list for remaining steps.



## Running the Experiment## Building



```bash```bash

# Build everythingmake clean

make clean && makemake

```

# Run full experiment (as root for CPU pinning and governor)

sudo ./run_all.sh## Usage



# Analyze results### Run both noop and lookup tests (recommended):

python3 analyze_baseline.py```bash

sudo ./run_comparison.sh

# Generate plots with error bars```

python3 plot_results.py

### Test individual variants:

# Validate operation counter (in another terminal while loader running)```bash

sudo ./validate_iterations.sh./test_step1.sh bench_noop     # Baseline

```./test_step1.sh bench_lookup   # Lookup test

```

## Comparing with Hot Cache

### Analyze results (from host VM, not QEMU):

After running both experiments:

**Step 1: Run analysis (no extra dependencies):**

```bash```bash

# From phase_I directorycd /path/to/array_paper

cd ..python3 analyze_results_simple.py results/

python3 compare_hot_cold.py  # Compare array_paper vs array_cold```

```

**Step 2: Generate plots (requires matplotlib):**

## Files```bash

python3 plot_results.py

- `array_bench.kern.c` - BPF programs with random key generation```

- `bench_loader.c` - Loads and attaches BPF programs

- `bench_run.c` - Userspace measurement driverThis generates 4 plots in `results/plots/`:

- `run_all.sh` - Automation script- `comparison_boxplot.png` - Performance comparison across variants

- `analyze_baseline.py` - Statistical analysis- `warmup_effect.png` - Iteration-by-iteration performance (cache effects)

- `plot_results.py` - Visualization with error bars- `overhead_comparison.png` - Pure map overhead (relative to noop baseline)

- `read_counter.c` - Operation counter validation tool- `distribution.png` - Performance distribution histogram



## Key Code Changes**Alternative: Full analysis with pandas (optional):**

```bash

**Noop baseline:**pip3 install pandas seaborn  # One-time install

```cpython3 analyze_results.py results/

u32 key = bpf_get_prandom_u32() % 4096;  // Generate random key```

asm volatile("" : : "r"(key));           // Prevent optimization

```### Manual testing:

```bash

**Lookup variants:**# Terminal 1: Load BPF program

```csudo ./bench_loader bench_lookup

u32 key = bpf_get_prandom_u32() % 4096;  // Random key for cold cache

struct value_Xb *value = bpf_map_lookup_elem(&bench_array_Xb, &key);# Terminal 2: Run benchmark

```./bench_run 1024 5 bench_lookup

```

This creates cache misses and tests realistic performance when data isn't in cache.

## Current Measurements

The benchmark:
- Calls `sys_bpfprof()` 1024 times (matches paper)
- Repeats 5 times (matches paper)
- Each BPF program invocation does ONE operation
- Measures total time and computes ns/operation
- Saves timestamped results to `results/` directory
- Results have 0666 permissions for host VM access

**Expected Output:**
```
Results saved to: results/bench_lookup_20241031_120000.csv
```

## Analysis Features

The `analyze_results.py` script provides:
1. **Summary statistics** - mean, std, min, max for each variant
2. **Overhead analysis** - pure map operation cost (variant - noop)
3. **Warm-up visualization** - shows cache effects across iterations
4. **Comparison plots** - boxplots and bar charts

**Example overhead calculation:**
```
noop baseline:     ~550 ns  (syscall + trampoline)
lookup test:       ~600 ns
Pure lookup cost:  ~50 ns   (600 - 550)
```

## Next Steps

1. ✅ **Step 1**: Basic lookup - DONE
2. **Step 4**: Add noop baseline - verify baseline subtraction works
3. **Step 6**: Add update operation with value storage map
4. **Step 7**: Add cold cache testing (cache flush between calls)
5. **Step 8**: Value size sweep (8B to 4KB)
6. **Step 9**: Per-CPU arrays
7. **Step 10**: Generate plots matching Figure 3-a to 3-d
