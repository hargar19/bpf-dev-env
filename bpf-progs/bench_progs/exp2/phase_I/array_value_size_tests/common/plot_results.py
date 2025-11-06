#!/usr/bin/env python3
"""
Create plots from combined_results.csv
Requires only matplotlib (no pandas needed)
"""

import os
import sys
import csv
from collections import defaultdict
import statistics

def load_combined_results(filepath='results/combined_results.csv'):
    """Load the combined results CSV.
    Supports both old format (ns_per_call) and new format (ns_per_operation).
    The new analysis script writes: variant,iteration,total_ns,ns_per_operation
    """
    if not os.path.exists(filepath):
        print(f"Error: {filepath} not found")
        print("Run analyze_results_simple.py first to generate it")
        sys.exit(1)

    data = defaultdict(lambda: {'ns_per_operation': [], 'by_iteration': defaultdict(list)})

    with open(filepath, 'r') as f:
        reader = csv.DictReader(f)
        # Determine which column to use
        use_col = None
        if 'ns_per_operation' in reader.fieldnames:
            use_col = 'ns_per_operation'
        elif 'ns_per_call' in reader.fieldnames:  # backward compatibility
            use_col = 'ns_per_call'
        else:
            print("Error: neither 'ns_per_operation' nor 'ns_per_call' column found in CSV")
            print(f"Columns present: {reader.fieldnames}")
            sys.exit(1)

        for row in reader:
            try:
                variant = row['variant']
                iteration = int(row['iteration'])
                val = float(row[use_col])
            except Exception as e:
                print(f"  Skipping row due to parse error: {row} ({e})")
                continue

            data[variant]['ns_per_operation'].append(val)
            data[variant]['by_iteration'][iteration].append(val)

    # Convert to regular dict
    for variant in data:
        data[variant]['by_iteration'] = dict(data[variant]['by_iteration'])

    return dict(data)

def create_plots(data, output_dir='results/plots'):
    """Generate all plots with error bars (standard deviation)"""
    try:
        import matplotlib
        matplotlib.use('Agg')  # Non-interactive backend
        import matplotlib.pyplot as plt
        import numpy as np
    except ImportError:
        print("\nError: matplotlib not installed")
        print("Install with: pip3 install matplotlib")
        sys.exit(1)
    
    os.makedirs(output_dir, exist_ok=True)
    
    variants = sorted(data.keys())
    colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b']
    
    # Plot 1: Bar plot with error bars (mean ± std dev)
    print("Creating bar plot with error bars...")
    plt.figure(figsize=(12, 6))
    
    means = [statistics.mean(data[v]['ns_per_operation']) for v in variants]
    stds = [statistics.stdev(data[v]['ns_per_operation']) if len(data[v]['ns_per_operation']) > 1 else 0 
            for v in variants]
    
    x_pos = range(len(variants))
    bars = plt.bar(x_pos, means, yerr=stds, capsize=5, alpha=0.7, 
                    color=colors[:len(variants)], edgecolor='black', linewidth=1.2)
    
    plt.xlabel('Variant', fontsize=12, fontweight='bold')
    plt.ylabel('Latency (ns per operation)', fontsize=12, fontweight='bold')
    plt.title('BPF Map Lookup Performance (Mean ± Std Dev)', fontsize=14, fontweight='bold')
    plt.xticks(x_pos, variants, rotation=45, ha='right')
    plt.grid(axis='y', alpha=0.3, linestyle='--')
    plt.tight_layout()
    
    output_path = os.path.join(output_dir, 'comparison_bar.png')
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"  Saved: {output_path}")
    plt.close()
    
    # Plot 2: Box plot comparison
    print("Creating box plot...")
    plt.figure(figsize=(12, 6))
    
    plot_data = [data[v]['ns_per_operation'] for v in variants]
    bp = plt.boxplot(plot_data, labels=variants, patch_artist=True)
    
    # Color boxes
    for patch, color in zip(bp['boxes'], colors):
        patch.set_facecolor(color)
        patch.set_alpha(0.6)
    
    plt.ylabel('Nanoseconds per operation', fontsize=12)
    plt.xlabel('Variant', fontsize=12)
    plt.title('BPF Array Map Performance Comparison', fontsize=14, fontweight='bold')
    plt.xticks(rotation=45, ha='right')
    plt.grid(True, alpha=0.3, axis='y')
    plt.tight_layout()
    
    plot_path = os.path.join(output_dir, 'comparison_boxplot.png')
    plt.savefig(plot_path, dpi=150, bbox_inches='tight')
    print(f"  ✓ Saved: {plot_path}")
    plt.close()
    
    # Plot 2: Warm-up effect (iteration by iteration)
    print("Creating warm-up effect plot...")
    plt.figure(figsize=(12, 6))
    
    for idx, variant in enumerate(variants):
        by_iter = data[variant]['by_iteration']
        iterations = sorted(by_iter.keys())
        means = [statistics.mean(by_iter[i]) for i in iterations]
        plt.plot(iterations, means, marker='o', linewidth=2,
                 markersize=8, label=variant, color=colors[idx % len(colors)])
    
    plt.xlabel('Iteration Number', fontsize=12)
    plt.ylabel('Nanoseconds per operation', fontsize=12)
    plt.title('Warm-up Effect: Performance Across Iterations', fontsize=14, fontweight='bold')
    plt.legend(fontsize=10)
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    
    plot_path = os.path.join(output_dir, 'warmup_effect.png')
    plt.savefig(plot_path, dpi=150, bbox_inches='tight')
    print(f"  ✓ Saved: {plot_path}")
    plt.close()
    
    # Plot 3: Overhead bar chart (if noop exists)
    noop_variants = [v for v in variants if 'noop' in v.lower()]
    if noop_variants:
        print("Creating overhead comparison plot...")
        noop_variant = noop_variants[0]
        noop_mean = statistics.mean(data[noop_variant]['ns_per_operation'])

        overhead_variants = [v for v in variants if v != noop_variant]
        overhead_values = []

        for v in overhead_variants:
            mean_ns = statistics.mean(data[v]['ns_per_operation'])
            overhead_ns = mean_ns - noop_mean
            overhead_values.append(overhead_ns)
        
        plt.figure(figsize=(10, 6))
        bars = plt.bar(overhead_variants, overhead_values, color=colors[:len(overhead_variants)], alpha=0.7)
        
        plt.ylabel('Overhead (ns)', fontsize=12)
        plt.xlabel('Variant', fontsize=12)
        plt.title(f'Pure Map Operation Overhead\n(baseline: {noop_variant} = {noop_mean:.2f} ns)', 
                 fontsize=14, fontweight='bold')
        plt.axhline(y=0, color='black', linestyle='-', linewidth=0.8)
        plt.xticks(rotation=45, ha='right')
        plt.grid(True, alpha=0.3, axis='y')
        
        # Add value labels on bars
        for bar, val in zip(bars, overhead_values):
            height = bar.get_height()
            plt.text(bar.get_x() + bar.get_width()/2., height,
                    f'{val:.1f}',
                    ha='center', va='bottom' if val >= 0 else 'top',
                    fontsize=10, fontweight='bold')
        
        plt.tight_layout()
        
        plot_path = os.path.join(output_dir, 'overhead_comparison.png')
        plt.savefig(plot_path, dpi=150, bbox_inches='tight')
        print(f"  ✓ Saved: {plot_path}")
        plt.close()
    
    # Plot 4: Distribution histogram
    print("Creating distribution histogram...")
    plt.figure(figsize=(12, 6))
    
    for idx, variant in enumerate(variants):
        plt.hist(data[variant]['ns_per_operation'], bins=20, alpha=0.5,
                 label=variant, color=colors[idx % len(colors)])
    
    plt.xlabel('Nanoseconds per operation', fontsize=12)
    plt.ylabel('Frequency', fontsize=12)
    plt.title('Performance Distribution', fontsize=14, fontweight='bold')
    plt.legend(fontsize=10)
    plt.grid(True, alpha=0.3, axis='y')
    plt.tight_layout()
    
    plot_path = os.path.join(output_dir, 'distribution.png')
    plt.savefig(plot_path, dpi=150, bbox_inches='tight')
    print(f"  ✓ Saved: {plot_path}")
    plt.close()
    
    # Plot 5: Throughput comparison (bar graph)
    print("Creating throughput comparison bar graph...")
    plt.figure(figsize=(12, 6))
    
    throughput_data = []
    for variant in variants:
        mean_ns = statistics.mean(data[variant]['ns_per_operation'])
        # Convert ns/op to Million ops/sec (1e9 ns/sec / mean_ns / 1e6)
        throughput_mops = (1e9 / mean_ns) / 1e6
        throughput_data.append(throughput_mops)
    
    bars = plt.bar(variants, throughput_data, color=colors[:len(variants)], alpha=0.7, edgecolor='black')
    
    plt.ylabel('Throughput (Million ops/sec)', fontsize=12)
    plt.xlabel('Variant', fontsize=12)
    plt.title('Throughput Comparison Across Variants', fontsize=14, fontweight='bold')
    plt.xticks(rotation=45, ha='right')
    plt.grid(True, alpha=0.3, axis='y')
    
    # Add value labels on bars
    for bar, val in zip(bars, throughput_data):
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.2f}',
                ha='center', va='bottom',
                fontsize=10, fontweight='bold')
    
    plt.tight_layout()
    
    plot_path = os.path.join(output_dir, 'throughput_comparison.png')
    plt.savefig(plot_path, dpi=150, bbox_inches='tight')
    print(f"  ✓ Saved: {plot_path}")
    plt.close()

    # Plot 6: Figure 3-a style value-size vs net cost (baseline-subtracted)
    noop_variants = [v for v in variants if 'noop' in v.lower()]
    if noop_variants:
        noop = noop_variants[0]
        noop_mean = statistics.mean(data[noop]['ns_per_operation'])
        # Extract lookup variants with sizes
        size_order = []  # list of tuples (label, size_bytes, net_ns, raw_mean)
        for v in variants:
            if v == noop:
                continue
            if 'lookup' in v:
                # parse size from name
                # expected suffix: _8b, _64b, _256b, _1kb, _4kb
                part = v.split('_')[-1]
                size_bytes = None
                if part.endswith('b') and part[:-1].isdigit():
                    size_bytes = int(part[:-1])
                elif part.endswith('kb') and part[:-2].isdigit():
                    size_bytes = int(part[:-2]) * 1024
                else:
                    continue
                raw_mean = statistics.mean(data[v]['ns_per_operation'])
                net = raw_mean - noop_mean
                size_order.append((v, size_bytes, net, raw_mean))
        if size_order:
            size_order.sort(key=lambda x: x[1])
            labels = [f"{s[1]//1024}KB" if s[1] >= 1024 else f"{s[1]}B" for s in size_order]
            net_vals = [s[2] for s in size_order]
            raw_vals = [s[3] for s in size_order]

            import matplotlib.pyplot as plt
            plt.figure(figsize=(8,5))
            bars = plt.bar(labels, net_vals, color='#1f77b4', alpha=0.75, edgecolor='black')
            plt.axhline(0, color='black', linewidth=0.8)
            plt.ylabel('Net ns per operation (minus noop)')
            plt.xlabel('Value size')
            plt.title('Figure 3-a Style: Array Lookup Cost vs Value Size (Hot)')
            for bar, net, raw in zip(bars, net_vals, raw_vals):
                txt = f"net={net:.3f}\nraw={raw:.3f}"
                plt.text(bar.get_x()+bar.get_width()/2, bar.get_height(), txt,
                         ha='center', va='bottom', fontsize=9)
            plt.tight_layout()
            plot_path = os.path.join(output_dir, 'figure3a_style.png')
            plt.savefig(plot_path, dpi=150, bbox_inches='tight')
            print(f"  ✓ Saved: {plot_path}")
            print("    (Note: negative nets indicate measurement noise larger than true lookup cost.)")
            plt.close()

def main():
    csv_path = 'results/combined_results.csv'
    if len(sys.argv) > 1:
        csv_path = sys.argv[1]
    
    print("=" * 60)
    print("Generating plots from combined results")
    print("=" * 60)
    
    data = load_combined_results(csv_path)
    print(f"Loaded data for {len(data)} variants")
    
    create_plots(data)
    
    print("\n" + "=" * 60)
    print("✓ All plots generated successfully!")
    print("=" * 60)
    print("\nPlots saved in: results/plots/")
    print("  - comparison_boxplot.png")
    print("  - warmup_effect.png")
    print("  - overhead_comparison.png")
    print("  - distribution.png")
    print("  - throughput_comparison.png")

if __name__ == '__main__':
    main()
