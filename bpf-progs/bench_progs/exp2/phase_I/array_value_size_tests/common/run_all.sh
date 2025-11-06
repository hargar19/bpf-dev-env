#!/bin/bash
# Run all benchmark variants using common infrastructure

set -e

COMMON_DIR="../common"

# Setup CPU governor using common script
echo "Setting up CPU governor..."
sudo ${COMMON_DIR}/setup_governor.sh

# Array of benchmark variants
variants=("noop" "8b" "64b" "256b" "1kb" "4kb")

# Create results directory
mkdir -p results

echo "Running all benchmark variants..."
for variant in "${variants[@]}"; do
    echo ""
    echo "========================================="
    echo "Running benchmark: bench_lookup_${variant}"
    echo "========================================="
    
    # Load and attach BPF program
    sudo ./bench_loader bench_lookup_${variant}
    
    # Run measurements
    sudo taskset -c 0 ./bench_run 1024 100 5 > results/bench_lookup_${variant}.csv
    
    echo "✓ Completed: bench_lookup_${variant}"
done

echo ""
echo "========================================="
echo "All benchmarks complete!"
echo "========================================="
echo "Results saved in: results/"
echo ""
echo "Next steps:"
echo "  1. Validate operation count: sudo ./read_counter"
echo "  2. Analyze results: python3 ${COMMON_DIR}/analyze_baseline.py"
echo "  3. Generate plots: python3 ${COMMON_DIR}/plot_results.py"
