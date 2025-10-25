#!/usr/bin/env python3
"""Parse Phase I array map throughput result files and produce:
- Summary table (stdout)
- CSV file (array_results_summary.csv)
- Bar chart PNG (array_results_plot.png)

Input: results_array_*.txt files in a target directory (default: ./results).
Optional arg1: path to results directory.
"""
import re
import glob
import os
import csv
import sys
from statistics import mean
try:
    import matplotlib.pyplot as plt
except ImportError:
    plt = None

# Match both legacy and new format with optional warm-up exclusion clause
# Examples:
# Total: 14072114 calls in 10.000 s (excl. 3.000s warm-up) = 1407211 calls/sec
# Total: 14072114 calls in 10.000 s = 1407211 calls/sec
TOTAL_RE = re.compile(
    r"Total:\s+(\d+)\s+calls\s+in\s+([0-9.]+)\s+s(?:\s+\(excl\.\s+[0-9.]+s\s+warm-up\))?\s+=\s+([0-9]+)\s+calls/sec"
)
INTERVAL_RE = re.compile(r"([0-9.]+):(\d+)")

def pick_results_dir():
    # Priority: explicit arg (deprecated), script sibling 'results', CWD 'results'
    if len(sys.argv) > 1 and sys.argv[1] != "results":
        d = sys.argv[1]
        if os.path.isdir(d):
            return d
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidate1 = os.path.join(script_dir, "results")
    if os.path.isdir(candidate1):
        return candidate1
    candidate2 = os.path.join(os.getcwd(), "results")
    if os.path.isdir(candidate2):
        return candidate2
    print("No 'results' directory found next to script or in CWD; please create one or pass path explicitly.")
    sys.exit(1)

target_dir = pick_results_dir()

pattern = os.path.join(target_dir, "results_array_*.txt")
files = sorted(glob.glob(pattern))
if not files:
    print("No results_array_*.txt files found.")
    sys.exit(1)

records = []
for f in files:
    with open(f) as fh:
        content = fh.read().strip().splitlines()
    total_line = None
    intervals = []
    for line in content:
        m_total = TOTAL_RE.search(line)
        if m_total:
            total_line = m_total
        else:
            m_int = INTERVAL_RE.search(line)
            if m_int:
                intervals.append(int(m_int.group(2)))
    if not total_line:
        print(f"Warning: no total line in {f}")
        continue
    total_calls = int(total_line.group(1))
    duration = float(total_line.group(2))
    rate = int(total_line.group(3))
    base = os.path.basename(f)
    variant = base.replace("results_", "").replace(".txt", "")
    avg_interval = mean(intervals) if intervals else 0
    records.append({
        "variant": variant,
        "total_calls": total_calls,
        "duration": duration,
        "rate": rate,
        "avg_interval_calls": int(avg_interval),
        "num_intervals": len(intervals)
    })

# Sort by rate descending
records.sort(key=lambda r: r["rate"], reverse=True)

# Compute relative slowdown vs fastest variant
max_rate = records[0]["rate"]
for r in records:
    r["pct_of_fastest"] = round(100.0 * r["rate"] / max_rate, 2)
    r["slowdown_pct"] = round(100.0 - r["pct_of_fastest"], 2)

# Print summary table
print("Variant,Calls/sec,% of fastest,Slowdown %,Total Calls,Avg Interval Calls,Intervals")
for r in records:
    print(f"{r['variant']},{r['rate']},{r['pct_of_fastest']},{r['slowdown_pct']},{r['total_calls']},{r['avg_interval_calls']},{r['num_intervals']}")

# Write CSV
csv_path = os.path.join(target_dir, "array_results_summary.csv")
try:
    with open(csv_path, "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["variant","rate","pct_of_fastest","slowdown_pct","total_calls","avg_interval_calls","num_intervals"])
        for r in records:
            writer.writerow([r['variant'], r['rate'], r['pct_of_fastest'], r['slowdown_pct'], r['total_calls'], r['avg_interval_calls'], r['num_intervals']])
    print(f"Wrote {csv_path}")
except PermissionError:
    # Fallback: attempt to remove root-owned file then retry; if still fails, write alt name
    print(f"PermissionError writing {csv_path}; attempting fallback...")
    alt_path = os.path.join(target_dir, f"array_results_summary_{os.getuid()}.csv")
    try:
        if os.path.exists(csv_path):
            os.remove(csv_path)
        with open(csv_path, "w", newline="") as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(["variant","rate","pct_of_fastest","slowdown_pct","total_calls","avg_interval_calls","num_intervals"])
            for r in records:
                writer.writerow([r['variant'], r['rate'], r['pct_of_fastest'], r['slowdown_pct'], r['total_calls'], r['avg_interval_calls'], r['num_intervals']])
        print(f"Wrote {csv_path} after removing old file")
    except Exception as e2:
        print(f"Second attempt failed: {e2}; writing to alt path {alt_path}")
        with open(alt_path, "w", newline="") as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(["variant","rate","pct_of_fastest","slowdown_pct","total_calls","avg_interval_calls","num_intervals"])
            for r in records:
                writer.writerow([r['variant'], r['rate'], r['pct_of_fastest'], r['slowdown_pct'], r['total_calls'], r['avg_interval_calls'], r['num_intervals']])
        print(f"Wrote {alt_path}")

# Plot if matplotlib available
if plt:
    plt.figure(figsize=(10,5))
    variants = [r['variant'] for r in records]
    rates = [r['rate'] for r in records]
    colors = ['#4C72B0' if i==0 else '#55A868' for i in range(len(rates))]
    plt.bar(variants, rates, color=colors)
    plt.ylabel('Calls per second')
    plt.title('Phase I Array Map Throughput (Variants)')
    plt.xticks(rotation=25, ha='right')
    for i,(v,rate) in enumerate(zip(variants,rates)):
        plt.text(i, rate*1.01, str(rate), ha='center', va='bottom', fontsize=8)
    plt.tight_layout()
    png_path = os.path.join(target_dir, 'array_results_plot.png')
    try:
        plt.savefig(png_path)
        print(f"Saved {png_path}")
    except PermissionError:
        alt_png = os.path.join(target_dir, f"array_results_plot_{os.getuid()}.png")
        print(f"PermissionError saving {png_path}; attempting alt file {alt_png}")
        plt.savefig(alt_png)
        print(f"Saved {alt_png}")
else:
    print("matplotlib not installed; skipping plot. Install and re-run: pip install matplotlib")
