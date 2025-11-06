#!/usr/bin/env python3
"""Compute baseline-subtracted per-operation costs.

Reads results/combined_results.csv (variant,iteration,total_ns,ns_per_syscall,ns_per_operation).
Identifies the noop variant (first containing 'noop').
Prints table:
  variant | mean_ns/op_raw | mean_ns/op_net (minus noop) | pct_over_noop | std_raw

Outputs CSV: results/baseline_adjusted.csv
"""
import csv, os, sys, statistics

IN_PATH = 'results/combined_results.csv'
OUT_PATH = 'results/baseline_adjusted.csv'

if len(sys.argv) > 1:
    IN_PATH = sys.argv[1]

if not os.path.exists(IN_PATH):
    print(f"Input CSV not found: {IN_PATH}")
    sys.exit(1)

rows = []
with open(IN_PATH) as f:
    r = csv.DictReader(f)
    for row in r:
        try:
            rows.append({
                'variant': row['variant'],
                'ns_per_operation': float(row['ns_per_operation'])
            })
        except Exception as e:
            print(f"Skipping row due to parse error: {row} ({e})")

if not rows:
    print('No data rows parsed.')
    sys.exit(1)

variants = {}
for r in rows:
    variants.setdefault(r['variant'], []).append(r['ns_per_operation'])

noop_variant = None
for v in variants:
    if 'noop' in v.lower():
        noop_variant = v
        break

if noop_variant is None:
    print('No noop variant found; cannot compute baseline-subtracted values.')
    sys.exit(1)

noop_mean = statistics.mean(variants[noop_variant])

summary = []
for v, vals in sorted(variants.items()):
    mean_raw = statistics.mean(vals)
    std_raw = statistics.pstdev(vals) if len(vals) > 1 else 0.0
    net = mean_raw - noop_mean if v != noop_variant else 0.0
    pct = (net / noop_mean * 100.0) if v != noop_variant else 0.0
    summary.append({
        'variant': v,
        'mean_ns_per_op_raw': mean_raw,
        'mean_ns_per_op_net': net,
        'pct_over_noop': pct,
        'std_raw': std_raw
    })

# Print table
print(f"Baseline variant: {noop_variant} mean={noop_mean:.4f} ns/op")
print("\nVariant                          Raw(ns/op)   Net(ns/op)   Overhead(%)   Std(ns)")
print("-" * 74)
for s in summary:
    print(f"{s['variant']:<30} {s['mean_ns_per_op_raw']:.4f}    {s['mean_ns_per_op_net']:.4f}      {s['pct_over_noop']:.2f}%       {s['std_raw']:.4f}")

# Write CSV
with open(OUT_PATH, 'w', newline='') as out:
    w = csv.writer(out)
    w.writerow(['variant','mean_ns_per_op_raw','mean_ns_per_op_net','pct_over_noop','std_raw','baseline_variant','baseline_mean_ns_per_op'])
    for s in summary:
        w.writerow([s['variant'], f"{s['mean_ns_per_op_raw']:.6f}", f"{s['mean_ns_per_op_net']:.6f}", f"{s['pct_over_noop']:.2f}", f"{s['std_raw']:.6f}", noop_variant, f"{noop_mean:.6f}"])

print(f"\nWrote baseline-adjusted CSV: {OUT_PATH}")
