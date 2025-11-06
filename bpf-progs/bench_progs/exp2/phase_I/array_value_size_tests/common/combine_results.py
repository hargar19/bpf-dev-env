#!/usr/bin/env python3
import csv, glob, os, statistics, sys

# Look for results in current working directory (where script is run from)
RESULTS_DIR = 'results'
out_path = os.path.join(RESULTS_DIR, 'combined_results.csv')

# Match both timestamped (bench_*_*.csv) and non-timestamped (bench_*.csv) files
files = sorted(glob.glob(os.path.join(RESULTS_DIR, 'bench_*.csv')))
if not files:
    print('[combine_results] No per-variant CSV files found (pattern bench_*.csv in results/).')
    sys.exit(1)

rows = []
for f in files:
    with open(f) as fh:
        reader = csv.reader(fh)
        header = None
        for line in reader:
            if not line:
                continue
            if line[0].startswith('#'):
                continue
            if header is None:
                header = line
                # Support three formats:
                # Bare metal (cycles): variant,iteration,total_cycles,cycles_per_syscall,cycles_per_operation,ns_per_operation
                # VM (time): variant,iteration,total_ns,ns_per_syscall,ns_per_operation
                # Legacy: variant,iteration,total_ns,ns_per_syscall,ns_per_operation
                continue
            # Flexible parsing based on column count
            if len(line) == 6:  # Bare metal format with cycles
                rows.append({
                    'variant': line[0],
                    'iteration': int(line[1]),
                    'total_cycles': float(line[2]),
                    'cycles_per_syscall': float(line[3]),
                    'cycles_per_operation': float(line[4]),
                    'ns_per_operation': float(line[5]),
                    'source_file': os.path.basename(f),
                    'has_cycles': True
                })
            elif len(line) == 5:  # VM format (time-based)
                rows.append({
                    'variant': line[0],
                    'iteration': int(line[1]),
                    'total_cycles': 0.0,  # not applicable
                    'cycles_per_syscall': 0.0,
                    'cycles_per_operation': 0.0,
                    'ns_per_operation': float(line[4]),
                    'source_file': os.path.basename(f),
                    'has_cycles': False
                })
            else:
                print(f'[combine_results] Skipping malformed line in {f}: {line}')
                continue

if not rows:
    print('[combine_results] No data rows parsed.')
    sys.exit(1)

# Write combined file
with open(out_path, 'w', newline='') as out:
    w = csv.writer(out)
    # Check if we have cycle data (bare metal measurements)
    has_cycles = any(r.get('has_cycles', False) for r in rows)
    if has_cycles:
        w.writerow(['variant','iteration','total_cycles','cycles_per_syscall','cycles_per_operation','ns_per_operation'])
        for r in rows:
            if r.get('has_cycles', False):
                w.writerow([r['variant'], r['iteration'], 
                           f"{r['total_cycles']:.0f}", 
                           f"{r['cycles_per_syscall']:.2f}", 
                           f"{r['cycles_per_operation']:.4f}",
                           f"{r['ns_per_operation']:.4f}"])
            else:
                # Fill with zeros for VM-based measurements in mixed dataset
                w.writerow([r['variant'], r['iteration'], 
                           "0", "0.00", "0.0000",
                           f"{r['ns_per_operation']:.4f}"])
    else:
        # All VM measurements - simpler format
        w.writerow(['variant','iteration','ns_per_operation'])
        for r in rows:
            w.writerow([r['variant'], r['iteration'], f"{r['ns_per_operation']:.4f}"])

# Print summary stats
by_variant = {}
for r in rows:
    by_variant.setdefault(r['variant'], []).append(r['ns_per_operation'])

print('[combine_results] Summary (ns_per_operation):')
for v, vals in by_variant.items():
    mean = statistics.mean(vals)
    std = statistics.pstdev(vals) if len(vals) > 1 else 0.0
    print(f"  {v:16s} count={len(vals):2d} mean={mean:.3f} std={std:.3f} min={min(vals):.3f} max={max(vals):.3f}")

print(f"[combine_results] Combined rows: {len(rows)} -> {out_path}")