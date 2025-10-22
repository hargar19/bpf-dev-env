#!/usr/bin/env python3
"""
Plot exp2 benchmark results comparing BPF map types and access patterns.
"""

import sys
import os
import re
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

def parse_result_file(filepath):
    """Parse a result file and extract average calls per interval."""
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Find all the timing:count pairs
    pattern = r'(\d+\.\d+):(\d+)'
    matches = re.findall(pattern, content)
    
    if not matches:
        return None
    
    # Calculate average
    counts = [int(count) for _, count in matches]
    avg = sum(counts) / len(counts)
    
    return avg

def parse_results_directory(results_dir):
    """Parse all result files in a directory."""
    results = {}
    
    results_path = Path(results_dir)
    if not results_path.exists():
        print(f"Error: Directory {results_dir} does not exist")
        return None
    
    # Expected test names
    test_files = [
        "Array-Lookup.txt",
        "Array-Update.txt", 
        "Array-Multi.txt",
        "PerCPU-Array-Lookup.txt",
        "PerCPU-Array-Update.txt",
        "Hash-Lookup.txt",
        "Hash-Update.txt",
        "Hash-Multi.txt",
        "PerCPU-Hash-Lookup.txt",
        "PerCPU-Hash-Update.txt",
        "LRU-Hash-Lookup.txt",
        "LRU-Hash-Update.txt",
        "LRU-PerCPU-Hash-Lookup.txt",
        "LRU-PerCPU-Hash-Update.txt"
    ]
    
    for test_file in test_files:
        filepath = results_path / test_file
        if filepath.exists():
            test_name = test_file.replace('.txt', '')
            avg = parse_result_file(filepath)
            if avg:
                results[test_name] = avg
        else:
            print(f"Warning: {test_file} not found")
    
    return results

def plot_comparison(results, output_file='exp2_comparison.png'):
    """Create comparison plots for the results."""
    
    # Create figure with subplots
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    fig.suptitle('Experiment 2: BPF Map Type Performance Comparison', fontsize=16, fontweight='bold')
    
    # Plot 1: All tests sorted by throughput
    ax1 = axes[0, 0]
    sorted_results = sorted(results.items(), key=lambda x: x[1], reverse=True)
    test_names = [name for name, _ in sorted_results]
    throughputs = [val / 1000 for _, val in sorted_results]  # Convert to thousands
    
    colors = []
    for name in test_names:
        if 'Array' in name and 'PerCPU' not in name:
            colors.append('#2E86AB')  # Blue
        elif 'PerCPU-Array' in name:
            colors.append('#A23B72')  # Purple
        elif 'Hash' in name and 'PerCPU' not in name and 'LRU' not in name:
            colors.append('#F18F01')  # Orange
        elif 'PerCPU-Hash' in name and 'LRU' not in name:
            colors.append('#C73E1D')  # Red
        elif 'LRU-Hash' in name and 'PerCPU' not in name:
            colors.append('#6A994E')  # Green
        else:  # LRU-PerCPU-Hash
            colors.append('#BC4B51')  # Dark red
    
    bars1 = ax1.barh(range(len(test_names)), throughputs, color=colors)
    ax1.set_yticks(range(len(test_names)))
    ax1.set_yticklabels(test_names, fontsize=9)
    ax1.set_xlabel('Throughput (K calls / 0.5s)', fontsize=11)
    ax1.set_title('All Tests Ranked by Throughput', fontsize=12, fontweight='bold')
    ax1.grid(axis='x', alpha=0.3)
    
    # Add values on bars
    for i, (bar, val) in enumerate(zip(bars1, throughputs)):
        ax1.text(val + 5, i, f'{val:.1f}K', va='center', fontsize=8)
    
    # Plot 2: Average by Map Type
    ax2 = axes[0, 1]
    map_types = {
        'Array': [],
        'PerCPU\nArray': [],
        'Hash': [],
        'PerCPU\nHash': [],
        'LRU\nHash': [],
        'LRU PerCPU\nHash': []
    }
    
    for name, val in results.items():
        if name.startswith('Array-'):
            map_types['Array'].append(val)
        elif name.startswith('PerCPU-Array'):
            map_types['PerCPU\nArray'].append(val)
        elif name.startswith('Hash-'):
            map_types['Hash'].append(val)
        elif name.startswith('PerCPU-Hash'):
            map_types['PerCPU\nHash'].append(val)
        elif name.startswith('LRU-Hash'):
            map_types['LRU\nHash'].append(val)
        elif name.startswith('LRU-PerCPU-Hash'):
            map_types['LRU PerCPU\nHash'].append(val)
    
    map_avgs = {k: np.mean(v) / 1000 if v else 0 for k, v in map_types.items()}
    map_stds = {k: np.std(v) / 1000 if v else 0 for k, v in map_types.items()}
    
    map_names = list(map_avgs.keys())
    map_avg_vals = list(map_avgs.values())
    map_std_vals = list(map_stds.values())
    
    bar_colors = ['#2E86AB', '#A23B72', '#F18F01', '#C73E1D', '#6A994E', '#BC4B51']
    bars2 = ax2.bar(range(len(map_names)), map_avg_vals, yerr=map_std_vals, 
                    color=bar_colors, capsize=5, alpha=0.8)
    ax2.set_xticks(range(len(map_names)))
    ax2.set_xticklabels(map_names, fontsize=10)
    ax2.set_ylabel('Throughput (K calls / 0.5s)', fontsize=11)
    ax2.set_title('Average Throughput by Map Type', fontsize=12, fontweight='bold')
    ax2.grid(axis='y', alpha=0.3)
    
    # Add values on bars
    for i, (bar, val) in enumerate(zip(bars2, map_avg_vals)):
        ax2.text(i, val + map_std_vals[i] + 5, f'{val:.1f}K', ha='center', fontsize=9)
    
    # Plot 3: By Access Pattern
    ax3 = axes[1, 0]
    patterns = {
        'Lookup': [],
        'Update': [],
        'Multi': []
    }
    
    for name, val in results.items():
        if name.endswith('-Lookup'):
            patterns['Lookup'].append(val)
        elif name.endswith('-Update'):
            patterns['Update'].append(val)
        elif name.endswith('-Multi'):
            patterns['Multi'].append(val)
    
    pattern_avgs = {k: np.mean(v) / 1000 if v else 0 for k, v in patterns.items()}
    pattern_stds = {k: np.std(v) / 1000 if v else 0 for k, v in patterns.items()}
    
    pattern_names = list(pattern_avgs.keys())
    pattern_avg_vals = list(pattern_avgs.values())
    pattern_std_vals = list(pattern_stds.values())
    
    bars3 = ax3.bar(range(len(pattern_names)), pattern_avg_vals, yerr=pattern_std_vals,
                    color=['#2E86AB', '#F18F01', '#6A994E'], capsize=10, alpha=0.8)
    ax3.set_xticks(range(len(pattern_names)))
    ax3.set_xticklabels(pattern_names, fontsize=11)
    ax3.set_ylabel('Throughput (K calls / 0.5s)', fontsize=11)
    ax3.set_title('Average Throughput by Access Pattern', fontsize=12, fontweight='bold')
    ax3.grid(axis='y', alpha=0.3)
    
    # Add values and sample counts
    for i, (bar, val, std) in enumerate(zip(bars3, pattern_avg_vals, pattern_std_vals)):
        count = len(patterns[pattern_names[i]])
        ax3.text(i, val + std + 5, f'{val:.1f}K\n(n={count})', ha='center', fontsize=9)
    
    # Plot 4: Throughput in calls/second with comparison
    ax4 = axes[1, 1]
    
    # Calculate calls per second
    map_avg_cps = {k: v * 2 for k, v in map_avgs.items()}  # 0.5s interval -> multiply by 2
    
    map_names_cps = list(map_avg_cps.keys())
    map_vals_cps = [v / 1000 for v in map_avg_cps.values()]  # Convert to millions
    
    bars4 = ax4.bar(range(len(map_names_cps)), map_vals_cps, 
                    color=bar_colors, alpha=0.8)
    ax4.set_xticks(range(len(map_names_cps)))
    ax4.set_xticklabels(map_names_cps, fontsize=10)
    ax4.set_ylabel('Throughput (M calls / second)', fontsize=11)
    ax4.set_title('Throughput in Calls per Second', fontsize=12, fontweight='bold')
    ax4.grid(axis='y', alpha=0.3)
    ax4.axhline(y=1.2, color='red', linestyle='--', linewidth=2, alpha=0.7, label='~1.2M avg')
    ax4.legend()
    
    # Add values on bars
    for i, (bar, val) in enumerate(zip(bars4, map_vals_cps)):
        ax4.text(i, val + 0.01, f'{val:.2f}M', ha='center', fontsize=9)
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Graph saved to: {output_file}")
    
    return output_file

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 plot_results.py <results_directory> [output_file]")
        print("Example: python3 plot_results.py results_20251022_052735/")
        sys.exit(1)
    
    results_dir = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else 'exp2_comparison.png'
    
    print(f"Parsing results from: {results_dir}")
    results = parse_results_directory(results_dir)
    
    if not results:
        print("Error: No results found")
        sys.exit(1)
    
    print(f"Found {len(results)} test results")
    print("\nResults summary:")
    for name, val in sorted(results.items(), key=lambda x: x[1], reverse=True):
        print(f"  {name:30s}: {val:10.1f} calls/0.5s ({val*2/1e6:.2f}M calls/s)")
    
    print(f"\nGenerating graphs...")
    plot_comparison(results, output_file)
    print("Done!")

if __name__ == '__main__':
    main()
