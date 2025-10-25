# Phase I Array Map Throughput – Run Guide

This guide explains how to build, run, and collect results for each array map variant.

## Directory
`exp2/phase_I/array/`

Files:
- `array_perf.kern.c` – BPF program with 6 variants
- `load_array` – User-space loader (selects one variant)
- `bench` – Syscall throughput generator (test syscall: 470)
- `RUN_GUIDE.md` – This guide

## Variants (Program Names)
1. `array_lookup_only`          – Single lookup helper
2. `array_lookup_update`        – Lookup + update helpers
3. `array_update_only`          – Single update helper (overwrite)
4. `array_multiple_lookups`     – 4 lookup helpers per invocation
5. `array_sequential_update`    – Round-robin key + update helper
6. `array_random_update`        – Pseudo-random key + update helper

## Build
Inside QEMU container:
```bash
cd /bpf-dev-env/bpf-progs/bench_progs/exp2/phase_I/array
make clean && make
```

Artifacts produced:
- `array_perf.bpf.o` / `array_perf.skel.h`
- `load_array`
- `bench`

## Running a Single Test
Pattern (updated bench usage with warm-up support):
```bash
./load_array <program_name> &
LOADER_PID=$!
./bench <measure_seconds> <interval_seconds> <warmup_seconds> <results_file>
kill $LOADER_PID
```
Arguments:
- `measure_seconds`: duration counted toward throughput (excludes warm-up)
- `interval_seconds`: reporting granularity during measurement phase
- `warmup_seconds`: warm-up period excluded from counts (can be 0)
- `results_file`: destination file (optional but recommended)

Example (lookup-only, 10s measured, 3s warm-up, 0.5s intervals):
```bash
mkdir -p results
./load_array array_lookup_only &
LOADER_PID=$!
./bench 30 0.5 3 results/results_array_lookup_only.txt
kill $LOADER_PID
```
`results/results_array_lookup_only.txt` will contain interval lines (only post warm-up) and final total line.

## Running All Variants (Full Suite)
With warm-up (3s) and 10s measured duration:
```bash
mkdir -p results
for p in array_lookup_only array_update_only array_lookup_update array_multiple_lookups array_sequential_update array_random_update; do
  echo "Running $p";
  ./load_array $p &
  LOADER_PID=$!
  ./bench 60 0.5 10 results/results_${p}.txt
  kill $LOADER_PID
  sleep 1
done
```
Longer stabilized run (30s measure, 5s warm-up) for two key variants:
```bash
./load_array array_lookup_only &; LOADER_PID=$!; ./bench 30 0.5 5 results/results_array_lookup_only_long.txt; kill $LOADER_PID
./load_array array_multiple_lookups &; LOADER_PID=$!; ./bench 30 0.5 5 results/results_array_multiple_lookups_long.txt; kill $LOADER_PID
```

## Result File Format
Each file (in `results/`) begins with a header:
```
# array map throughput test warmup=<warmup_seconds> total=<measure_seconds> interval=<interval_seconds>
```
Followed by interval lines (only measurement phase):
```
<elapsed_since_measure_start>:<calls_in_interval>
```
Final total line:
```
Total: <calls> calls in <measure_seconds> s (excl. <warmup_seconds>s warm-up) = <rate> calls/sec
```
Parse totals:
```bash
grep "Total:" results/results_array_*.txt | awk '{print $2, $7, $10}'
```
(Fields: calls, measured_seconds, calls/sec; adjust if shell splitting differs.)
