# Phase I: BPF Map Performance Experiments

Comprehensive performance characterization of eBPF map operations, replicating experiments from "Understanding Performance of eBPF Maps" paper.

## Experiment Categories

### 📊 Array Value Size Tests
**Directory:** `array_value_size_tests/`

Tests how **value size affects performance** of BPF array maps.

**Experiments:**
- Lookup operations: hot cache vs cold cache
- Update operations: hot cache vs cold cache
- Value sizes: 8B, 64B, 256B, 1KB, 4KB

**Research Questions:**
- Does value size affect latency?
- How do cache effects change with size?
- What's the update overhead for different sizes?

See [array_value_size_tests/README.md](array_value_size_tests/README.md) for details.

---

## Future Experiments (To Be Added)

### 🔢 Map Size Tests
Test how **map size affects performance** (Figure 6 in paper).
- Fixed value size (e.g., 64B)
- Varying map sizes: 1K, 4K, 16K, 64K entries
- Shows cache pressure effects

### 🔑 Per-CPU Arrays
Test **per-CPU array performance** (Figure 5 in paper).
- Compare BPF_MAP_TYPE_ARRAY vs BPF_MAP_TYPE_PERCPU_ARRAY
- Shows benefit of CPU-local storage

### #️⃣ Hash Map Tests
Test **hash map performance** (Figures 3c-d in paper).
- BPF_MAP_TYPE_HASH
- Compare with array performance
- Test hash collision effects

---

## Quick Start

### Run Array Value Size Tests
```bash
cd array_value_size_tests/

# Run individual experiment
cd array_lookup_hot/
make clean && make
sudo ./run_all.sh
python3 ../common/analyze_baseline.py
python3 ../common/plot_results.py

# Run all experiments
for dir in array_lookup_hot array_lookup_cold array_update_hot array_update_cold; do
    cd $dir && make clean && make && sudo ./run_all.sh && cd ..
done

# Compare all results
python3 compare_all_experiments.py
```

---

## Directory Structure

```
phase_I/
├── README.md (this file)
│
└── array_value_size_tests/
    ├── README.md
    ├── common/                      # Shared infrastructure
    ├── array_lookup_hot/            # Hot cache lookups
    ├── array_lookup_cold/           # Cold cache lookups
    ├── array_update_hot/            # Hot cache updates
    ├── array_update_cold/           # Cold cache updates
    ├── compare_all_experiments.py   # Comprehensive comparison
    └── *.md                         # Documentation
```

---

## Infrastructure

All experiments share common infrastructure in `common/`:
- **C files**: `bench_loader.c`, `bench_run.c`, `read_counter.c`
- **Python**: `analyze_baseline.py`, `plot_results.py`, `combine_results.py`
- **Shell**: `setup_governor.sh`, `test_counter.sh`, etc.

This eliminates duplication and ensures consistency across experiments.

---

## Measurement Methodology

- **Custom syscall**: `bpfprof` (number 451) triggers BPF programs
- **Timing**: Auto-detects VM (clock_gettime) vs bare metal (RDTSC)
- **Iterations**: 1024 syscalls × 100 repeats × 100 operations = 10.24M operations
- **Statistics**: Mean, standard deviation, coefficient of variation
- **CPU control**: Pinned to CPU 0, performance governor

---

## Paper Reference

"Understanding Performance of eBPF Maps"
- Figure 3: Array lookups (hot/cold cache)
- Figure 4: Array updates (hot/cold cache)
- Figure 5: Per-CPU arrays
- Figure 6: Map size effects
- Figure 7+: Hash maps

---

## Status

✅ **Completed**: Array value size tests (4 experiments)
- Hot/cold cache lookups
- Hot/cold cache updates
- Comprehensive comparison

⏳ **Planned**: Map size tests, per-CPU arrays, hash maps
