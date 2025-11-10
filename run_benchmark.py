"""
Run performance benchmarks
Tests encryption/communication overhead with varying configurations
"""

import sys
import os
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from logger.performance_logger import SystemBenchmark


def main():
    parser = argparse.ArgumentParser(description='Performance Benchmark Suite')
    parser.add_argument('--config', default='./config/system_config.yaml', help='Configuration file')
    parser.add_argument('--output', default='./results', help='Output directory')
    parser.add_argument('--quick', action='store_true', help='Run quick benchmark (shorter durations)')
    
    args = parser.parse_args()
    
    print("Starting Performance Benchmark Suite\n")
    
    benchmark = SystemBenchmark(args.config)
    
    if args.quick:
        print("Running quick benchmark...\n")
        # Quick versions with fewer tests and shorter durations
        benchmark.benchmark_varying_meters([2, 5, 10], duration_sec=10)
        benchmark.benchmark_varying_intervals([1, 10, 30], num_meters=5, duration_sec=10)
    else:
        print("Running full benchmark suite...\n")
        benchmark.run_full_benchmark()
    
    print(f"\nResults saved to {args.output}")


if __name__ == '__main__':
    main()
