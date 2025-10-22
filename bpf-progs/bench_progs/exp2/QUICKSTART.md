# Experiment 2 Quick Start Guide

## Prerequisites

1. **Kernel with bpfprof syscall** must be running
2. **Experiment 1 baseline** should be completed for comparison
3. **Build tools** must be available (clang, bpftool, gcc, libbpf)

## Build

```bash
cd /home/hargar/bpf/bpf-env/profiling/bpf-dev-env/bpf-progs/bench_progs/exp2
make
```

This will generate:
- `*.kern.o` - Compiled BPF programs
- `*.skel.h` - BPF skeleton headers
- `trigger.user` - Single syscall test
- `bench.user` - Throughput benchmark
- `load.user` - BPF loader + benchmark runner
- `load_stats_enabled.user` - Loader with BPF stats enabled

## Running Tests

### Option 1: Run All Tests Automatically

```bash
./run_all_tests.sh
```

This will:
- Run all 14 map type/pattern combinations
- Each test runs for 30 seconds
- Save results to `results_YYYYMMDD_HHMMSS/` directory
- Generate a summary markdown file

### Option 2: Run Individual Tests

```bash
# Test array lookup
./load.user null_tp_array_lookup.kern.o

# Test hash update
./load.user null_tp_hash_update.kern.o

# Test with BPF stats enabled
./load_stats_enabled.user null_tp_percpu_array_lookup.kern.o
```

This generates:
- Throughput comparison table (sorted by performance)
- Overhead analysis vs baseline
- Pattern analysis (lookup vs update vs multi)
- Map type comparison
- Statistical analysis (mean, median, stdev, CV)

## Analyzing Results

### View Summary

```bash
# After running run_all_tests.sh
cat results_*/SUMMARY.md
```

### Detailed Analysis

```bash
# Run Python analysis script
./analyze_results.py results_YYYYMMDD_HHMMSS/
```

### Manual Analysis

```bash
# View individual test results
cat results_*/Array-Lookup.txt

# Compare all tests
grep -h "^[0-9]" results_*/*.txt | awk -F: '{sum+=$2; count++} END {print sum/count}'

# Find best performing test
for f in results_*/*.txt; do
    avg=$(grep -h "^[0-9]" "$f" | awk -F: '{sum+=$2; count++} END {print sum/count}')
    echo "$avg $f"
done | sort -rn | head -1
```

## Test Matrix

| Map Type | Lookup | Update | Multi | Total |
|----------|--------|--------|-------|-------|
| Array | ✓ | ✓ | ✓ | 3 |
| PerCPU Array | ✓ | ✓ | - | 2 |
| Hash | ✓ | ✓ | ✓ | 3 |
| PerCPU Hash | ✓ | ✓ | - | 2 |
| LRU Hash | ✓ | ✓ | - | 2 |
| LRU PerCPU Hash | ✓ | ✓ | - | 2 |
| **Total** | | | | **14 tests** |

## Expected Results Order (Fastest to Slowest)

1. **PerCPU Array Lookup** - No locks, direct index
2. **PerCPU Array Update** - No locks, simple write
3. **Array Lookup** - Direct index, minimal contention
4. **Array Update** - Direct index + atomic op
5. **PerCPU Hash Lookup** - Hash compute, no locks
6. **PerCPU Hash Update** - Hash + write, no locks
7. **Hash Lookup** - Hash compute + locks
8. **Hash Update** - Hash + locks + atomic
9. **LRU PerCPU Hash Lookup** - Hash + LRU, no global locks
10. **LRU PerCPU Hash Update** - Hash + LRU + write
11. **LRU Hash Lookup** - Hash + LRU + locks
12. **LRU Hash Update** - Hash + LRU + locks + atomic
13. **Array Multi** - 4× array operations
14. **Hash Multi** - 4× hash operations

## Comparison to Exp1

Use exp1 results as baseline for pure tracing overhead:

```bash
# Example calculation
EXP1_BASELINE=2500000  # calls per 30 seconds from exp1
EXP2_ARRAY=2300000     # calls per 30 seconds from exp2 array test
MAP_OVERHEAD=$(( (EXP1_BASELINE - EXP2_ARRAY) * 100 / EXP1_BASELINE ))
echo "Map access overhead: ${MAP_OVERHEAD}%"
```

## Next Steps

After completing exp2 BPF tests:

1. **Analyze results** - Which map types have acceptable overhead?
2. **Kernel module comparison** - Implement same tests with kernel modules
3. **Static kernel patches** - Add inline instrumentation
4. **Compare all approaches** - BPF vs module vs patch overhead
5. **Real workloads** - Test with actual data processing in BPF programs

## Files Reference

- `README.md` - Full experiment documentation
- `QUICKSTART.md` - This file
- `run_all_tests.sh` - Automated test runner
- `analyze_results.py` - Python analysis script
- `null_tp_*.kern.c` - BPF programs (14 variants)
- `bench.user.c` - Throughput benchmark
- `load.user.c` - BPF loader
- `Makefile` - Build system
