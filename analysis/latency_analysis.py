#!/usr/bin/env python3
"""
Latency Analysis and Visualization Script
for Low-Latency Order Book Simulator

This script analyzes performance data from the order book simulator
and generates visualizations for latency distributions, throughput,
and performance characteristics.
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import argparse
import sys
from pathlib import Path
import warnings
warnings.filterwarnings('ignore')

# Set style for professional-looking plots
plt.style.use('seaborn-v0_8-darkgrid')
sns.set_palette("husl")

class LatencyAnalyzer:
    """Analyzes latency data from the order book simulator."""
    
    def __init__(self, csv_file):
        """Initialize analyzer with CSV data file."""
        self.csv_file = Path(csv_file)
        self.data = None
        self.trade_data = None
        self.load_data()
    
    def load_data(self):
        """Load performance and trade data from CSV files."""
        try:
            # Load performance data
            if self.csv_file.exists():
                self.data = pd.read_csv(self.csv_file)
                print(f"Loaded {len(self.data)} performance measurements")
            else:
                print(f"Performance file {self.csv_file} not found")
                return
            
            # Try to load trade data
            trade_file = Path("data") / "simulation_trades.csv"
            if trade_file.exists():
                self.trade_data = pd.read_csv(trade_file)
                print(f"Loaded {len(self.trade_data)} trade records")
            else:
                print(f"Trade file {trade_file} not found")
                
        except Exception as e:
            print(f"Error loading data: {e}")
            sys.exit(1)
    
    def calculate_statistics(self):
        """Calculate comprehensive latency statistics."""
        if self.data is None:
            print("No data loaded")
            return None
        
        stats = {}
        
        # Overall statistics
        stats['overall'] = {
            'count': len(self.data),
            'mean_ns': self.data['latency_ns'].mean(),
            'median_ns': self.data['latency_ns'].median(),
            'std_ns': self.data['latency_ns'].std(),
            'min_ns': self.data['latency_ns'].min(),
            'max_ns': self.data['latency_ns'].max(),
            'p95_ns': self.data['latency_ns'].quantile(0.95),
            'p99_ns': self.data['latency_ns'].quantile(0.99),
            'p999_ns': self.data['latency_ns'].quantile(0.999)
        }
        
        # Convert to microseconds for readability
        for key in ['mean', 'median', 'std', 'min', 'max', 'p95', 'p99', 'p999']:
            stats['overall'][f'{key}_us'] = stats['overall'][f'{key}_ns'] / 1000.0
        
        # Per-operation statistics
        if 'operation_type' in self.data.columns:
            for op_type in self.data['operation_type'].unique():
                op_data = self.data[self.data['operation_type'] == op_type]
                stats[op_type] = {
                    'count': len(op_data),
                    'mean_ns': op_data['latency_ns'].mean(),
                    'median_ns': op_data['latency_ns'].median(),
                    'std_ns': op_data['latency_ns'].std(),
                    'p95_ns': op_data['latency_ns'].quantile(0.95),
                    'p99_ns': op_data['latency_ns'].quantile(0.99)
                }
                
                # Convert to microseconds
                for key in ['mean', 'median', 'std', 'p95', 'p99']:
                    stats[op_type][f'{key}_us'] = stats[op_type][f'{key}_ns'] / 1000.0
        
        return stats
    
    def plot_latency_distribution(self, save_path=None):
        """Plot latency distribution histogram."""
        if self.data is None:
            return
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
        
        # Histogram in nanoseconds
        ax1.hist(self.data['latency_ns'], bins=100, alpha=0.7, edgecolor='black')
        ax1.set_xlabel('Latency (nanoseconds)')
        ax1.set_ylabel('Frequency')
        ax1.set_title('Latency Distribution (Nanoseconds)')
        ax1.axvline(self.data['latency_ns'].mean(), color='red', linestyle='--', 
                   label=f'Mean: {self.data["latency_ns"].mean():.1f} ns')
        ax1.axvline(self.data['latency_ns'].median(), color='orange', linestyle='--', 
                   label=f'Median: {self.data["latency_ns"].median():.1f} ns')
        ax1.legend()
        ax1.grid(True, alpha=0.3)
        
        # Histogram in microseconds (zoomed)
        latency_us = self.data['latency_ns'] / 1000.0
        ax2.hist(latency_us, bins=100, alpha=0.7, edgecolor='black', color='green')
        ax2.set_xlabel('Latency (microseconds)')
        ax2.set_ylabel('Frequency')
        ax2.set_title('Latency Distribution (Microseconds)')
        ax2.axvline(latency_us.mean(), color='red', linestyle='--', 
                   label=f'Mean: {latency_us.mean():.3f} μs')
        ax2.axvline(latency_us.median(), color='orange', linestyle='--', 
                   label=f'Median: {latency_us.median():.3f} μs')
        ax2.legend()
        ax2.grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Latency distribution plot saved to {save_path}")
        
        plt.show()
    
    def plot_latency_percentiles(self, save_path=None):
        """Plot latency percentiles."""
        if self.data is None:
            return
        
        percentiles = np.arange(0, 100.1, 0.1)
        latency_percentiles = np.percentile(self.data['latency_ns'], percentiles)
        
        plt.figure(figsize=(12, 8))
        
        # Main percentile plot
        plt.subplot(2, 1, 1)
        plt.plot(percentiles, latency_percentiles, linewidth=2, color='blue')
        plt.xlabel('Percentile')
        plt.ylabel('Latency (nanoseconds)')
        plt.title('Latency Percentiles')
        plt.grid(True, alpha=0.3)
        
        # Highlight key percentiles
        key_percentiles = [50, 90, 95, 99, 99.9]
        for p in key_percentiles:
            value = np.percentile(self.data['latency_ns'], p)
            plt.axhline(y=value, color='red', linestyle='--', alpha=0.7)
            plt.text(p, value, f'P{p}: {value:.1f}ns', rotation=90, 
                    verticalalignment='bottom', fontsize=8)
        
        # Log scale for tail percentiles
        plt.subplot(2, 1, 2)
        plt.semilogy(percentiles, latency_percentiles, linewidth=2, color='red')
        plt.xlabel('Percentile')
        plt.ylabel('Latency (nanoseconds, log scale)')
        plt.title('Latency Percentiles (Log Scale)')
        plt.grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Latency percentiles plot saved to {save_path}")
        
        plt.show()
    
    def plot_operation_comparison(self, save_path=None):
        """Compare latencies across different operation types."""
        if self.data is None or 'operation_type' not in self.data.columns:
            return
        
        plt.figure(figsize=(12, 8))
        
        # Box plot comparison
        plt.subplot(2, 1, 1)
        operation_types = self.data['operation_type'].unique()
        data_by_operation = [self.data[self.data['operation_type'] == op]['latency_ns'] 
                           for op in operation_types]
        
        plt.boxplot(data_by_operation, labels=operation_types)
        plt.ylabel('Latency (nanoseconds)')
        plt.title('Latency Distribution by Operation Type')
        plt.xticks(rotation=45)
        plt.grid(True, alpha=0.3)
        
        # Violin plot for better distribution visualization
        plt.subplot(2, 1, 2)
        sns.violinplot(data=self.data, x='operation_type', y='latency_ns')
        plt.ylabel('Latency (nanoseconds)')
        plt.title('Latency Distribution by Operation Type (Violin Plot)')
        plt.xticks(rotation=45)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Operation comparison plot saved to {save_path}")
        
        plt.show()
    
    def plot_throughput_analysis(self, save_path=None):
        """Analyze throughput over time."""
        if self.data is None:
            return
        
        # Calculate throughput in windows
        window_size = 1000  # operations per window
        windows = []
        throughputs = []
        
        for i in range(0, len(self.data), window_size):
            window_data = self.data.iloc[i:i+window_size]
            if len(window_data) > 0:
                total_time_ns = window_data['latency_ns'].sum()
                if total_time_ns > 0:
                    throughput = len(window_data) * 1e9 / total_time_ns  # ops/sec
                    windows.append(i)
                    throughputs.append(throughput)
        
        plt.figure(figsize=(12, 6))
        plt.plot(windows, throughputs, linewidth=2, marker='o', markersize=4)
        plt.xlabel('Operation Number')
        plt.ylabel('Throughput (operations/second)')
        plt.title('Throughput Over Time')
        plt.grid(True, alpha=0.3)
        
        # Add average line
        avg_throughput = np.mean(throughputs)
        plt.axhline(y=avg_throughput, color='red', linestyle='--', 
                   label=f'Average: {avg_throughput:.0f} ops/sec')
        plt.legend()
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Throughput analysis plot saved to {save_path}")
        
        plt.show()
    
    def plot_trade_analysis(self, save_path=None):
        """Analyze trade data if available."""
        if self.trade_data is None:
            print("No trade data available for analysis")
            return
        
        fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15, 12))
        
        # Trade volume over time
        trade_data = self.trade_data.copy()
        trade_data['timestamp'] = pd.to_datetime(trade_data['timestamp'])
        trade_data['cumulative_volume'] = trade_data['quantity'].cumsum()
        
        ax1.plot(trade_data.index, trade_data['cumulative_volume'], linewidth=2)
        ax1.set_xlabel('Trade Number')
        ax1.set_ylabel('Cumulative Volume')
        ax1.set_title('Cumulative Trade Volume')
        ax1.grid(True, alpha=0.3)
        
        # Price distribution
        ax2.hist(trade_data['price'], bins=50, alpha=0.7, edgecolor='black')
        ax2.set_xlabel('Trade Price')
        ax2.set_ylabel('Frequency')
        ax2.set_title('Trade Price Distribution')
        ax2.grid(True, alpha=0.3)
        
        # Trade size distribution
        ax3.hist(trade_data['quantity'], bins=50, alpha=0.7, edgecolor='black', color='green')
        ax3.set_xlabel('Trade Quantity')
        ax3.set_ylabel('Frequency')
        ax3.set_title('Trade Size Distribution')
        ax3.grid(True, alpha=0.3)
        
        # Price vs Quantity scatter
        ax4.scatter(trade_data['price'], trade_data['quantity'], alpha=0.6, s=20)
        ax4.set_xlabel('Trade Price')
        ax4.set_ylabel('Trade Quantity')
        ax4.set_title('Price vs Quantity Relationship')
        ax4.grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Trade analysis plot saved to {save_path}")
        
        plt.show()
    
    def generate_report(self, output_file=None):
        """Generate comprehensive performance report."""
        stats = self.calculate_statistics()
        if stats is None:
            return
        
        report = []
        report.append("=" * 60)
        report.append("LOW-LATENCY ORDER BOOK SIMULATOR - PERFORMANCE REPORT")
        report.append("=" * 60)
        report.append("")
        
        # Overall statistics
        overall = stats['overall']
        report.append("OVERALL PERFORMANCE STATISTICS:")
        report.append("-" * 40)
        report.append(f"Total Operations: {overall['count']:,}")
        report.append(f"Mean Latency: {overall['mean_us']:.3f} μs ({overall['mean_ns']:.1f} ns)")
        report.append(f"Median Latency: {overall['median_us']:.3f} μs ({overall['median_ns']:.1f} ns)")
        report.append(f"Standard Deviation: {overall['std_us']:.3f} μs ({overall['std_ns']:.1f} ns)")
        report.append(f"Min Latency: {overall['min_us']:.3f} μs ({overall['min_ns']:.1f} ns)")
        report.append(f"Max Latency: {overall['max_us']:.3f} μs ({overall['max_ns']:.1f} ns)")
        report.append(f"95th Percentile: {overall['p95_us']:.3f} μs ({overall['p95_ns']:.1f} ns)")
        report.append(f"99th Percentile: {overall['p99_us']:.3f} μs ({overall['p99_ns']:.1f} ns)")
        report.append(f"99.9th Percentile: {overall['p999_us']:.3f} μs ({overall['p999_ns']:.1f} ns)")
        report.append("")
        
        # Calculate throughput
        if overall['mean_ns'] > 0:
            theoretical_throughput = 1e9 / overall['mean_ns']
            report.append(f"Theoretical Max Throughput: {theoretical_throughput:,.0f} ops/sec")
            report.append("")
        
        # Per-operation statistics
        operation_stats = {k: v for k, v in stats.items() if k != 'overall'}
        if operation_stats:
            report.append("PER-OPERATION STATISTICS:")
            report.append("-" * 40)
            for op_type, op_stats in operation_stats.items():
                report.append(f"\n{op_type.upper()}:")
                report.append(f"  Operations: {op_stats['count']:,}")
                report.append(f"  Mean Latency: {op_stats['mean_us']:.3f} μs")
                report.append(f"  Median Latency: {op_stats['median_us']:.3f} μs")
                report.append(f"  95th Percentile: {op_stats['p95_us']:.3f} μs")
                report.append(f"  99th Percentile: {op_stats['p99_us']:.3f} μs")
        
        # Trade statistics
        if self.trade_data is not None:
            report.append("\n" + "=" * 60)
            report.append("TRADE EXECUTION STATISTICS:")
            report.append("-" * 40)
            report.append(f"Total Trades: {len(self.trade_data):,}")
            report.append(f"Total Volume: {self.trade_data['quantity'].sum():,}")
            report.append(f"Average Trade Size: {self.trade_data['quantity'].mean():.1f}")
            report.append(f"Price Range: {self.trade_data['price'].min()} - {self.trade_data['price'].max()}")
            report.append(f"Average Trade Price: {self.trade_data['price'].mean():.2f}")
        
        report.append("\n" + "=" * 60)
        report.append("END OF REPORT")
        report.append("=" * 60)
        
        report_text = "\n".join(report)
        print(report_text)
        
        if output_file:
            with open(output_file, 'w') as f:
                f.write(report_text)
            print(f"\nReport saved to {output_file}")
    
    def run_full_analysis(self, output_dir="analysis_output"):
        """Run complete analysis with all visualizations."""
        output_path = Path(output_dir)
        output_path.mkdir(exist_ok=True)
        
        print("Running comprehensive latency analysis...")
        print(f"Output directory: {output_path}")
        
        # Generate all plots
        self.plot_latency_distribution(output_path / "latency_distribution.png")
        self.plot_latency_percentiles(output_path / "latency_percentiles.png")
        self.plot_operation_comparison(output_path / "operation_comparison.png")
        self.plot_throughput_analysis(output_path / "throughput_analysis.png")
        
        if self.trade_data is not None:
            self.plot_trade_analysis(output_path / "trade_analysis.png")
        
        # Generate report
        self.generate_report(output_path / "performance_report.txt")
        
        print(f"\nAnalysis complete! All outputs saved to {output_path}")

def main():
    """Main entry point for the analysis script."""
    parser = argparse.ArgumentParser(
        description="Analyze latency data from the order book simulator"
    )
    parser.add_argument(
        "csv_file", 
        help="Path to the performance CSV file"
    )
    parser.add_argument(
        "--output-dir", 
        default="analysis_output",
        help="Output directory for analysis results (default: analysis_output)"
    )
    parser.add_argument(
        "--plots-only", 
        action="store_true",
        help="Generate only plots, skip text report"
    )
    parser.add_argument(
        "--report-only", 
        action="store_true",
        help="Generate only text report, skip plots"
    )
    
    args = parser.parse_args()
    
    # Initialize analyzer
    analyzer = LatencyAnalyzer(args.csv_file)
    
    if args.report_only:
        analyzer.generate_report()
    elif args.plots_only:
        analyzer.run_full_analysis(args.output_dir)
        print("Plots generated (skipping text report)")
    else:
        analyzer.run_full_analysis(args.output_dir)

if __name__ == "__main__":
    main()
