# Paper-Style Array Map Benchmark

Implementation of the methodology from "Understanding Performance of eBPF Maps" (ACM SIGCOMM 2024).

## Directory Structure

```
array_paper/
├── array_bench.kern.c    # BPF programs (noop, lookup, update)
├── bench_loader.c        # Loads and attaches BPF program
├── bench_run.c           # Userspace benchmark runner
├── Makefile              # Build system
├── test_step1.sh         # Test script for current step
└── README.md             # This file
```

## Implementation Steps

### ✅ Step 1: Basic BPF Lookup Program
- **Status**: COMPLETE
- **Files**: `array_bench.kern.c`, `bench_loader.c`, `bench_run.c`
- **What**: Single lookup operation, 4096-entry array, 8-byte values

### ✅ Step 4: Noop Baseline
- **Status**: COMPLETE
- **Files**: Same as Step 1
- **What**: Empty BPF program for measuring trampoline overhead

### ✅ Step 5: Results & Analysis
- **Status**: COMPLETE
- **Files**: `run_comparison.sh`, `analyze_results.py`
- **What**: Automated test runner, results saved to `results/`, analysis script for host VM
- **Test**: `sudo ./run_comparison.sh`

### Step 6-10: TODO
See todo list for remaining steps.

## Building

```bash
make clean
make
```

## Usage

### Run both noop and lookup tests (recommended):
```bash
sudo ./run_comparison.sh
```

### Test individual variants:
```bash
./test_step1.sh bench_noop     # Baseline
./test_step1.sh bench_lookup   # Lookup test
```

### Analyze results (from host VM, not QEMU):

**Step 1: Run analysis (no extra dependencies):**
```bash
cd /path/to/array_paper
python3 analyze_results_simple.py results/
```

**Step 2: Generate plots (requires matplotlib):**
```bash
python3 plot_results.py
```

This generates 4 plots in `results/plots/`:
- `comparison_boxplot.png` - Performance comparison across variants
- `warmup_effect.png` - Iteration-by-iteration performance (cache effects)
- `overhead_comparison.png` - Pure map overhead (relative to noop baseline)
- `distribution.png` - Performance distribution histogram

**Alternative: Full analysis with pandas (optional):**
```bash
pip3 install pandas seaborn  # One-time install
python3 analyze_results.py results/
```

### Manual testing:
```bash
# Terminal 1: Load BPF program
sudo ./bench_loader bench_lookup

# Terminal 2: Run benchmark
./bench_run 1024 5 bench_lookup
```

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
