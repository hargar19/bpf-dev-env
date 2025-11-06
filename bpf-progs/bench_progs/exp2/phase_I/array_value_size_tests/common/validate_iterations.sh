#!/usr/bin/env bash
# Validation script to check operation counter after running benchmarks
# Usage: Run this WHILE bench_loader is running (in another terminal)

echo "=== Operation Counter Validation ==="
echo ""

if [ ! -e /sys/fs/bpf/op_counter ]; then
    echo "ERROR: Operation counter not found at /sys/fs/bpf/op_counter"
    echo "Make sure bench_loader is running before using this script."
    exit 1
fi

echo "Reading operation counter..."
./read_counter

echo ""
echo "=== Validation Info ==="
echo "Expected operations per run:"
echo "  - 1024 syscalls per repeat"
echo "  - 100 operations per syscall"
echo "  - 100 repeats"
echo "  - Total: 1024 × 100 × 100 = 10,240,000 operations per variant"
echo ""
echo "If the counter shows significantly less, the loop may not be fully executing."
