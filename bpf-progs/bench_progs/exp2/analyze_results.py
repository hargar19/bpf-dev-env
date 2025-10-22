#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""
Experiment 2 Results Analyzer

Parses benchmark output files and generates comparative analysis:
- Throughput comparison across map types
- Overhead calculation vs exp1 baseline
- Statistical analysis (mean, std dev, variance)
"""

import sys
import os
import re
import statistics
from pathlib import Path
from collections import defaultdict

def parse_result_file(filepath):
    """Parse a single result file and extract call counts."""
    calls = []
    
    with open(filepath, 'r') as f:
        for line in f:
            # Format: timestamp:calls
            match = re.match(r'^(\d+\.\d+):(\d+)', line.strip())
            if match:
                timestamp = float(match.group(1))
                call_count = int(match.group(2))
                calls.append(call_count)
    
    return calls

def analyze_test(name, calls):
    """Calculate statistics for a test."""
    if not calls:
        return None
    
    return {
        'name': name,
        'count': len(calls),
        'mean': statistics.mean(calls),
        'median': statistics.median(calls),
        'stdev': statistics.stdev(calls) if len(calls) > 1 else 0,
        'min': min(calls),
        'max': max(calls),
        'total': sum(calls)
    }

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <results_directory>")
        print(f"Example: {sys.argv[0]} results_20241017_143000")
        sys.exit(1)
    
    results_dir = Path(sys.argv[1])
    
    if not results_dir.exists():
        print(f"ERROR: Results directory not found: {results_dir}")
        sys.exit(1)
    
    # Find all result files
    result_files = sorted(results_dir.glob("*.txt"))
    
    if not result_files:
        print(f"ERROR: No result files found in {results_dir}")
        sys.exit(1)
    
    print("=" * 80)
    print("Experiment 2: Map Access Overhead Analysis")
    print("=" * 80)
    print()
    
    # Parse all results
    all_stats = []
    
    for result_file in result_files:
        if result_file.name == "SUMMARY.md":
            continue
        
        test_name = result_file.stem
        calls = parse_result_file(result_file)
        
        if not calls:
            print(f"WARNING: No data found in {result_file.name}")
            continue
        
        stats = analyze_test(test_name, calls)
        if stats:
            all_stats.append(stats)
    
    if not all_stats:
        print("ERROR: No valid test results found")
        sys.exit(1)
    
    # Sort by mean throughput (descending)
    all_stats.sort(key=lambda x: x['mean'], reverse=True)
    
    # Print detailed table
    print("Throughput Comparison (sorted by mean calls/interval)")
    print("-" * 80)
    print(f"{'Test Name':<35} {'Mean':>10} {'Median':>10} {'StdDev':>10} {'CV %':>8}")
    print("-" * 80)
    
    for stats in all_stats:
        cv = (stats['stdev'] / stats['mean'] * 100) if stats['mean'] > 0 else 0
        print(f"{stats['name']:<35} {stats['mean']:>10.0f} {stats['median']:>10.0f} "
              f"{stats['stdev']:>10.1f} {cv:>7.2f}%")
    
    print()
    
    # Group by map type for comparison
    print("=" * 80)
    print("Overhead Analysis by Map Type")
    print("=" * 80)
    print()
    
    # Find baseline (highest throughput - should be lookup operations)
    baseline = all_stats[0]
    print(f"Baseline: {baseline['name']} = {baseline['mean']:.0f} calls/interval")
    print()
    
    print(f"{'Test Name':<35} {'Throughput':>12} {'vs Baseline':>12} {'Overhead':>10}")
    print("-" * 80)
    
    for stats in all_stats:
        throughput = stats['mean']
        vs_baseline = (throughput / baseline['mean'] * 100)
        overhead = ((baseline['mean'] - throughput) / baseline['mean'] * 100)
        
        print(f"{stats['name']:<35} {throughput:>12.0f} {vs_baseline:>11.1f}% {overhead:>9.1f}%")
    
    print()
    
    # Pattern analysis
    print("=" * 80)
    print("Pattern Analysis")
    print("=" * 80)
    print()
    
    # Group by operation type
    patterns = defaultdict(list)
    for stats in all_stats:
        if 'Lookup' in stats['name']:
            patterns['Lookup'].append(stats)
        elif 'Update' in stats['name']:
            patterns['Update'].append(stats)
        elif 'Multi' in stats['name']:
            patterns['Multi'].append(stats)
    
    for pattern, tests in patterns.items():
        if not tests:
            continue
        
        print(f"{pattern} Operations:")
        avg_throughput = statistics.mean([t['mean'] for t in tests])
        print(f"  Average throughput: {avg_throughput:.0f} calls/interval")
        print(f"  Tests: {len(tests)}")
        
        # Sort by throughput
        tests.sort(key=lambda x: x['mean'], reverse=True)
        for t in tests:
            print(f"    {t['name']:<40} {t['mean']:>10.0f}")
        print()
    
    # Map type comparison
    print("=" * 80)
    print("Map Type Comparison")
    print("=" * 80)
    print()
    
    map_types = {
        'Array': [],
        'PerCPU-Array': [],
        'Hash': [],
        'PerCPU-Hash': [],
        'LRU-Hash': [],
        'LRU-PerCPU-Hash': []
    }
    
    for stats in all_stats:
        name = stats['name']
        for map_type in map_types.keys():
            if name.startswith(map_type):
                map_types[map_type].append(stats)
                break
    
    print(f"{'Map Type':<20} {'Avg Throughput':>15} {'Best':>12} {'Worst':>12}")
    print("-" * 80)
    
    for map_type, tests in map_types.items():
        if not tests:
            continue
        
        avg = statistics.mean([t['mean'] for t in tests])
        best = max([t['mean'] for t in tests])
        worst = min([t['mean'] for t in tests])
        
        print(f"{map_type:<20} {avg:>15.0f} {best:>12.0f} {worst:>12.0f}")
    
    print()
    print("=" * 80)
    print("Analysis complete!")
    print(f"Processed {len(all_stats)} tests from {results_dir}")
    print("=" * 80)

if __name__ == "__main__":
    main()
