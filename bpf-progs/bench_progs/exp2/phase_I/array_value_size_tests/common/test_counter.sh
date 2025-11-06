#!/usr/bin/env bash
# Quick test to validate operation counter

set -e

echo "=== Testing Operation Counter ==="
echo ""

# Clean up any existing pinned map
rm -f /sys/fs/bpf/op_counter

# Start loader in background
echo "[1] Loading bench_noop program..."
./bench_loader bench_noop &
LOADER_PID=$!

# Give it time to load and pin the map
sleep 2

# Run a small benchmark (10 iterations, 5 repeats)
echo "[2] Running 10 syscalls (should execute 1000 operations)..."
taskset -c 0 ./bench_run 10 5 bench_noop /dev/null > /dev/null

# Read the counter
echo "[3] Reading operation counter..."
./read_counter

# Expected: 10 syscalls × 100 ops/syscall × 5 repeats = 5000 operations
echo ""
echo "Expected: 5000 operations (10 syscalls × 100 ops × 5 repeats)"

# Kill the loader
echo ""
echo "[4] Cleaning up..."
kill $LOADER_PID 2>/dev/null || true
sleep 1

echo ""
echo "=== Test Complete ==="
