## Figure 3-a: Hot Cache Array Lookups

This experiment replicates **Figure 3-a** from "Understanding Performance of eBPF Maps" paper - measuring hot array map lookup performance across value sizes.

### Configuration

- **Cache Behavior**: Hot (single key=0, stays in L1 cache)
- **Map Type**: BPF_MAP_TYPE_ARRAY  
- **Value Sizes**: 8B, 64B, 256B, 1KB, 4KB
- **Operations**: 100 lookups per syscall × 1024 syscalls × 100 repeats

### Paper-Style Setup (Recommended)

For best results matching the paper's methodology:

1. **Configure CPU (run as root)**:
```bash
sudo ./setup_governor.sh
```
This sets:
- Performance governor (disables frequency scaling)
- Disables turbo boost
- Shows current CPU frequencies

2. **Run Experiment**:
```bash
./run_all.sh
```
This automatically:
- Builds all components
- Pins benchmark to CPU 0
- Runs 100 repeats per variant (paper-style sample size)
- Uses RDTSC cycle counters for high-precision timing
- Tests single hot key (key=0) for all value sizes
- Generates `results/combined_results.csv`

### Key Improvements Over Initial Setup

✓ **Increased sample size**: 100 repeats (was 15)  
✓ **CPU pinning**: Pinned to CPU 0 via taskset  
✓ **Performance governor**: Eliminates frequency scaling noise  
✓ **Cycle counter timing**: RDTSC instead of clock_gettime (nanosecond resolution → cycle precision)  
✓ **Single hot key**: key=0 for all lookups (maximally hot cache)  

### Build & Run
```bash
./run_all.sh
```
Generates per-variant CSVs and `results/combined_results.csv`.

### Baseline-Subtracted Analysis
```bash
python3 analyze_baseline.py
```
Produces human-readable table and `results/baseline_adjusted.csv`.

### Plots
```bash
python3 plot_results.py
```
Generates plots in `results/plots/`:
- comparison_boxplot.png
- warmup_effect.png
- overhead_comparison.png (noop baseline removed)
- distribution.png
- throughput_comparison.png

### Interpretation
New CSV format includes cycle counts:
- `total_cycles`: Raw cycle count for batch
- `cycles_per_syscall`: Cycles per syscall invocation
- `cycles_per_operation`: Cycles per single map lookup (÷1024)
- `ns_per_operation`: Nanoseconds per lookup (cycle/freq)

Baseline subtraction (`analyze_baseline.py`) removes noop overhead to approximate pure map helper cost.

### Environment Variables
Override defaults (if not using setup script):
```bash
ITERATIONS=1024 REPEATS=100 CPU_PIN=0 ./run_all.sh
```

### Manual CPU Setup (Alternative)
If you can't run setup_governor.sh as root:
```bash
# Set performance governor
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Disable turbo (Intel)
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo

# Check current frequencies
grep MHz /proc/cpuinfo | head -n 4
```

### Troubleshooting
- If `bpftool` not found, ensure kernel tree under `linux/` has been built/tools available.
- Missing tracepoint `sys_enter_bpfprof` means syscall not active; ensure your kernel includes custom syscall.
