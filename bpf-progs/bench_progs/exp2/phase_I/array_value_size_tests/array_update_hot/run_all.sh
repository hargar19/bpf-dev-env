#!/usr/bin/env bash
set -euo pipefail

# Paper-style experiment automation with CPU pinning and performance governor
# Replicates Figure 3-a: hot array lookups across value sizes

ITERATIONS=${ITERATIONS:-1024}
REPEATS=${REPEATS:-100}  # Increased to 100 for paper-style statistics
WARMUP=${WARMUP:-5}
CPU_PIN=${CPU_PIN:-0}     # Pin to CPU 0 by default

VARIANTS=(bench_noop bench_update_8b bench_update_64b bench_update_256b bench_update_1kb bench_update_4kb)

echo "[run_all] Paper-style experiment setup"
echo "[run_all] ========================================"
echo "[run_all] Iterations: $ITERATIONS"
echo "[run_all] Repeats: $REPEATS"
echo "[run_all] Warmup: $WARMUP"
echo "[run_all] CPU pinning: $CPU_PIN"
echo "[run_all] ========================================"

# Check if cpufreq is available (not present in QEMU/VM environments)
if [ -d /sys/devices/system/cpu/cpu0/cpufreq ]; then
    # Check if running as root for governor setup
    if [ "$(id -u)" -eq 0 ]; then
        echo "[run_all] Setting CPU governor to performance..."
        for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
            if [ -f "$gov" ]; then
                echo performance > "$gov" 2>/dev/null || echo "  (skipping $gov)"
            fi
        done
        echo "[run_all] Disabling turbo boost (Intel)..."
        if [ -f /sys/devices/system/cpu/intel_pstate/no_turbo ]; then
            echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null || true
        fi
        echo "[run_all] CPU frequency scaling configured"
    else
        echo "[run_all] Warning: Not running as root; cannot set CPU governor"
        echo "[run_all] For best results, run as root or manually set:"
        echo "[run_all]   echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor"
    fi
else
    echo "[run_all] Note: cpufreq not available (QEMU/VM environment)"
    echo "[run_all] Frequency scaling control not needed for virtual CPUs"
    echo "[run_all] Proceeding with high-precision cycle counting..."
fi

echo "[run_all] Building..."
make -s clean all

mkdir -p results

for v in "${VARIANTS[@]}"; do
  echo "[run_all] ========================================"
  echo "[run_all] Variant: $v"
  echo "[run_all] ========================================"
  ./bench_loader "$v" &
  LPID=$!
  # Give loader time to attach
  sleep 2
  
  # Pin benchmark to specified CPU using taskset
  echo "[run_all] Running with CPU pinning (CPU $CPU_PIN)..."
  taskset -c "$CPU_PIN" ./bench_run "$ITERATIONS" "$REPEATS" "$v"
  
  # Stop loader
  kill "$LPID" >/dev/null 2>&1 || true
  wait "$LPID" 2>/dev/null || true
  sleep 1
done

echo "[run_all] ========================================"
echo "[run_all] Combining results..."
python3 ../common/combine_results.py
echo "[run_all] Done. Combined file: results/combined_results.csv"
echo "[run_all] ========================================"