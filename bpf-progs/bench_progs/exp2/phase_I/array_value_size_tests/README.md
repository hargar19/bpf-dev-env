# Array Value Size Tests

This directory contains experiments measuring **how value size affects BPF array map performance**.

## Experiments

Tests 4 combinations of operation type × cache behavior:

| Directory | Operation | Cache | Map Size | Key Pattern |
|-----------|-----------|-------|----------|-------------|
| `array_lookup_hot` | Lookup | Hot | 4K entries | key=0 (constant) |
| `array_lookup_cold` | Lookup | Cold | 16K entries | Random keys |
| `array_update_hot` | Update | Hot | 4K entries | key=0 (constant) |
| `array_update_cold` | Update | Cold | 16K entries | Random keys |

Each experiment tests **5 value sizes**: 8B, 64B, 256B, 1KB, 4KB

## What We're Measuring

**Research Questions:**
1. Does value size affect lookup/update latency?
2. How do cache effects (hot vs cold) change with value size?
3. What's the overhead of updates vs lookups for different sizes?

**Key Variables:**
- **Value size**: 8B → 4KB (shows memcpy and cache line effects)
- **Operation type**: Lookup (read) vs Update (write)
- **Cache behavior**: Hot (same key) vs Cold (random keys)
- **Map size**: 4K (hot) vs 16K (cold) entries

## Directory Structure

```
array_value_size_tests/
├── common/                      # Shared infrastructure
│   ├── bench_loader.c          # BPF program loader
│   ├── bench_run.c             # Measurement driver
│   ├── read_counter.c          # Counter validation
│   ├── analyze_baseline.py     # Statistical analysis
│   ├── plot_results.py         # Visualization
│   └── ...
│
├── array_lookup_hot/           # Hot cache lookups
├── array_lookup_cold/          # Cold cache lookups
├── array_update_hot/           # Hot cache updates
├── array_update_cold/          # Cold cache updates
│
└── compare_all_experiments.py  # Comprehensive comparison
```

## Running Experiments

### Individual Experiment
```bash
cd array_lookup_hot/
make clean && make
sudo ./run_all.sh
python3 ../common/analyze_baseline.py
python3 ../common/plot_results.py
```

### All Experiments
```bash
# From array_value_size_tests/ directory
for dir in array_lookup_hot array_lookup_cold array_update_hot array_update_cold; do
    echo "Running $dir..."
    cd $dir && make clean && make && sudo ./run_all.sh && cd ..
done
```

### Compare All Results
```bash
# From array_value_size_tests/ directory
python3 compare_all_experiments.py
```

## Expected Results

**Hot Cache (key=0):**
- Lookup: Flat latency across all value sizes (~3-4 ns/op)
- Update: Linear increase with value size (memcpy overhead)

**Cold Cache (random keys):**
- Lookup: Significant increase for large values (cache misses)
- Update: Combined memcpy + cache miss penalty

**Update vs Lookup:**
- Small values (8B-64B): 2-5× slower (minimal memcpy)
- Large values (1KB-4KB): 10-80× slower (significant memcpy)

## Paper Reference

These experiments replicate:
- **Figure 3**: Array lookup performance (hot vs cold cache)
- **Figure 4**: Array update performance (hot vs cold cache)

From: "Understanding Performance of eBPF Maps"

## Key Findings

1. **Lookups are cache-sensitive**: 4KB cold lookup can be 12× slower than hot
2. **Updates are memcpy-bound**: 4KB update is 80× slower than 8B update (hot cache)
3. **Value size matters most for updates**: Lookup latency stays relatively flat in hot cache
4. **Cache effects amplify with size**: Larger values suffer more from cache misses
