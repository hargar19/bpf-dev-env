#!/bin/bash
# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
# Experiment 2 - Automated Map Overhead Test Runner
#
# This script runs all map type benchmarks and collects results.

set -e

# Configuration
DURATION=30
INTERVAL=0.5
RESULTS_DIR="results_$(date +%Y%m%d_%H%M%S)"

# Map test variants (BPF programs to test)
TESTS=(
    "null_tp_array_lookup.kern.o:Array-Lookup"
    "null_tp_array_update.kern.o:Array-Update"
    "null_tp_array_multi.kern.o:Array-Multi"
    "null_tp_percpu_array_lookup.kern.o:PerCPU-Array-Lookup"
    "null_tp_percpu_array_update.kern.o:PerCPU-Array-Update"
    "null_tp_hash_lookup.kern.o:Hash-Lookup"
    "null_tp_hash_update.kern.o:Hash-Update"
    "null_tp_hash_multi.kern.o:Hash-Multi"
    "null_tp_percpu_hash_lookup.kern.o:PerCPU-Hash-Lookup"
    "null_tp_percpu_hash_update.kern.o:PerCPU-Hash-Update"
    "null_tp_lru_hash_lookup.kern.o:LRU-Hash-Lookup"
    "null_tp_lru_hash_update.kern.o:LRU-Hash-Update"
    "null_tp_lru_percpu_hash_lookup.kern.o:LRU-PerCPU-Hash-Lookup"
    "null_tp_lru_percpu_hash_update.kern.o:LRU-PerCPU-Hash-Update"
)

# Create results directory
mkdir -p "$RESULTS_DIR"

echo "============================================"
echo "  Experiment 2: Map Access Overhead Tests"
echo "============================================"
echo "Duration: ${DURATION}s per test"
echo "Interval: ${INTERVAL}s"
echo "Results: $RESULTS_DIR"
echo "Total tests: ${#TESTS[@]}"
echo ""

# Function to run a single test
run_test() {
    local bpf_prog=$1
    local test_name=$2
    local output_file="${RESULTS_DIR}/${test_name}.txt"
    
    echo "Testing: $test_name"
    echo "  BPF Program: $bpf_prog"
    echo "  Output: $output_file"
    
    # Check if BPF program exists
    if [ ! -f "$bpf_prog" ]; then
        echo "  ERROR: BPF program not found!"
        echo "ERROR: $bpf_prog not found" > "$output_file"
        return 1
    fi
    
    # Run the test and capture output
    echo "=== $test_name ===" > "$output_file"
    echo "BPF Program: $bpf_prog" >> "$output_file"
    echo "Duration: ${DURATION}s" >> "$output_file"
    echo "Date: $(date)" >> "$output_file"
    echo "" >> "$output_file"
    
    ./load.user "$bpf_prog" 2>&1 | tee -a "$output_file"
    
    echo "  ✓ Complete"
    echo ""
}

# Check if programs are built
if [ ! -f "load.user" ]; then
    echo "ERROR: load.user not found. Please run 'make' first."
    exit 1
fi

# Run all tests
test_num=0
for test_spec in "${TESTS[@]}"; do
    test_num=$((test_num + 1))
    IFS=':' read -r bpf_prog test_name <<< "$test_spec"
    
    echo "[$test_num/${#TESTS[@]}] ----------------------------------------"
    run_test "$bpf_prog" "$test_name"
    
    # Short pause between tests
    sleep 2
done

# Generate summary report
SUMMARY_FILE="${RESULTS_DIR}/SUMMARY.md"
echo "# Experiment 2 Results Summary" > "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"
echo "Generated: $(date)" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"
echo "## Test Configuration" >> "$SUMMARY_FILE"
echo "- Duration: ${DURATION}s per test" >> "$SUMMARY_FILE"
echo "- Interval: ${INTERVAL}s" >> "$SUMMARY_FILE"
echo "- Total tests: ${#TESTS[@]}" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"
echo "## Results" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"
echo "| Test Name | Avg Calls/Interval | Result File |" >> "$SUMMARY_FILE"
echo "|-----------|-------------------|-------------|" >> "$SUMMARY_FILE"

# Extract average calls per interval from each result file
for test_spec in "${TESTS[@]}"; do
    IFS=':' read -r bpf_prog test_name <<< "$test_spec"
    result_file="${RESULTS_DIR}/${test_name}.txt"
    
    if [ -f "$result_file" ]; then
        # Calculate average from all intervals (format: timestamp:calls)
        avg_calls=$(grep -E "^[0-9]+\.[0-9]+:[0-9]+" "$result_file" | \
                    awk -F: '{sum+=$2; count++} END {if(count>0) printf "%.0f", sum/count; else print "N/A"}')
        echo "| $test_name | $avg_calls | $test_name.txt |" >> "$SUMMARY_FILE"
    else
        echo "| $test_name | ERROR | $test_name.txt |" >> "$SUMMARY_FILE"
    fi
done

echo "" >> "$SUMMARY_FILE"
echo "## Analysis" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"
echo "To analyze results:" >> "$SUMMARY_FILE"
echo '```bash' >> "$SUMMARY_FILE"
echo "# View individual test results" >> "$SUMMARY_FILE"
echo "cat $RESULTS_DIR/<test-name>.txt" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"
echo "# Compare throughput across tests" >> "$SUMMARY_FILE"
echo "grep -h \"Avg Calls\" $RESULTS_DIR/*.txt" >> "$SUMMARY_FILE"
echo '```' >> "$SUMMARY_FILE"

echo ""
echo "============================================"
echo "  All tests complete!"
echo "============================================"
echo "Results directory: $RESULTS_DIR"
echo "Summary: $SUMMARY_FILE"
echo ""
echo "To view summary:"
echo "  cat $SUMMARY_FILE"
echo ""
