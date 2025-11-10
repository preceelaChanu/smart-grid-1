"""
Logger and Performance Analysis Module
Tracks system performance metrics:
- Encryption times vs number of meters
- Encryption times vs data intervals
- Communication overhead
- Computation costs
- Memory usage
"""

import time
import json
import sys
import os
from typing import Dict, List, Optional
from dataclasses import asdict
import threading

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from common.utils import ConfigurationManager, FileManager, TimerContext
from smart_meters.meter_client import SmartMeterGrid
from analytics_server.server import AnalyticsServer


class PerformanceLogger:
    """
    Logs and analyzes system performance metrics
    """
    
    def __init__(self, log_dir: str = './logs'):
        """
        Initialize performance logger
        
        Args:
            log_dir: Directory for log files
        """
        self.log_dir = log_dir
        FileManager.ensure_directory(log_dir)
        
        self.system_log = f"{log_dir}/system.log"
        self.performance_log = f"{log_dir}/performance.json"
        self.results_log = f"{log_dir}/results.json"
        
        self.metrics = []
    
    def log_message(self, message: str, level: str = "INFO"):
        """
        Log a message
        
        Args:
            message: Message to log
            level: Log level (DEBUG, INFO, WARNING, ERROR)
        """
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        log_message = f"[{timestamp}] [{level}] {message}"
        print(log_message)
        FileManager.append_log(self.system_log, log_message)
    
    def log_metric(self, metric_data: Dict):
        """
        Log a performance metric
        
        Args:
            metric_data: Dictionary containing metric data
        """
        metric_data['timestamp'] = time.time()
        self.metrics.append(metric_data)
        FileManager.append_json_line(self.performance_log, metric_data)
    
    def save_results(self, results: Dict):
        """
        Save test results
        
        Args:
            results: Dictionary containing test results
        """
        FileManager.write_json(self.results_log, results)
        self.log_message(f"Results saved to {self.results_log}")


class SystemBenchmark:
    """
    Benchmarks the smart grid system under various conditions
    """
    
    def __init__(self, config_path: str = './config/system_config.yaml'):
        """
        Initialize benchmark
        
        Args:
            config_path: Path to system configuration file
        """
        self.config = ConfigurationManager.load_config(config_path)
        self.logger = PerformanceLogger()
        self.results = {}
    
    def benchmark_varying_meters(self, meter_counts: List[int], duration_sec: int = 30) -> Dict:
        """
        Benchmark system performance with varying number of meters
        Tests how encryption scales with more meters
        
        Args:
            meter_counts: List of meter counts to test
            duration_sec: Duration of each test
        
        Returns:
            Dictionary with benchmark results
        """
        self.logger.log_message("=== Benchmark: Varying Meter Counts ===")
        
        results = {}
        
        for num_meters in meter_counts:
            self.logger.log_message(f"Testing with {num_meters} meters for {duration_sec}s...")
            
            # Create server
            server = AnalyticsServer()
            server.start()
            
            # Create meter grid
            meter_config = self.config.get('smart_meters', {})
            server_config = self.config.get('analytics_server', {})
            
            grid = SmartMeterGrid(
                num_meters=num_meters,
                server_host=server_config.get('host', '127.0.0.1'),
                server_port=server_config.get('port', 5000),
                reading_interval_sec=meter_config.get('reading_interval_sec', 5),
                batch_size=meter_config.get('batch_size', 5)
            )
            
            grid.start()
            
            # Run test
            time.sleep(duration_sec)
            
            # Collect metrics
            grid_stats = grid.get_grid_statistics()
            server_stats = server.get_server_statistics()
            
            test_result = {
                'num_meters': num_meters,
                'duration_sec': duration_sec,
                'grid_stats': grid_stats,
                'server_stats': server_stats,
                'total_readings': server_stats['total_readings_received'],
                'throughput_per_meter': server_stats['total_readings_received'] / num_meters if num_meters > 0 else 0
            }
            
            results[f"{num_meters}_meters"] = test_result
            
            # Log metric
            self.logger.log_metric({
                'test': 'varying_meters',
                'num_meters': num_meters,
                'total_readings': test_result['total_readings'],
                'throughput_per_meter': test_result['throughput_per_meter']
            })
            
            # Cleanup
            grid.stop()
            server.stop()
            
            self.logger.log_message(f"✓ Test complete with {num_meters} meters: "
                                   f"{test_result['total_readings']} readings")
        
        self.results['varying_meters'] = results
        return results
    
    def benchmark_varying_intervals(self, 
                                    intervals_sec: List[float], 
                                    num_meters: int = 5,
                                    duration_sec: int = 30) -> Dict:
        """
        Benchmark system performance with varying data intervals
        Tests how intervals affect encryption and communication costs
        
        Args:
            intervals_sec: List of intervals to test (in seconds)
            num_meters: Number of meters for this test
            duration_sec: Duration of each test
        
        Returns:
            Dictionary with benchmark results
        """
        self.logger.log_message(f"=== Benchmark: Varying Data Intervals ({num_meters} meters) ===")
        
        results = {}
        
        for interval in intervals_sec:
            self.logger.log_message(f"Testing interval {interval}s for {duration_sec}s...")
            
            # Create server
            server = AnalyticsServer()
            server.start()
            
            # Create meter grid
            meter_config = self.config.get('smart_meters', {})
            server_config = self.config.get('analytics_server', {})
            
            grid = SmartMeterGrid(
                num_meters=num_meters,
                server_host=server_config.get('host', '127.0.0.1'),
                server_port=server_config.get('port', 5000),
                reading_interval_sec=interval,
                batch_size=meter_config.get('batch_size', 5)
            )
            
            grid.start()
            
            # Run test
            time.sleep(duration_sec)
            
            # Collect metrics
            grid_stats = grid.get_grid_statistics()
            server_stats = server.get_server_statistics()
            
            test_result = {
                'interval_sec': interval,
                'duration_sec': duration_sec,
                'num_meters': num_meters,
                'grid_stats': grid_stats,
                'server_stats': server_stats,
                'total_readings': server_stats['total_readings_received'],
                'avg_readings_per_sec': server_stats['total_readings_received'] / duration_sec if duration_sec > 0 else 0
            }
            
            results[f"{interval}s"] = test_result
            
            # Log metric
            self.logger.log_metric({
                'test': 'varying_intervals',
                'interval_sec': interval,
                'num_meters': num_meters,
                'total_readings': test_result['total_readings'],
                'readings_per_sec': test_result['avg_readings_per_sec']
            })
            
            # Cleanup
            grid.stop()
            server.stop()
            
            self.logger.log_message(f"✓ Test complete with {interval}s interval: "
                                   f"{test_result['total_readings']} readings, "
                                   f"{test_result['avg_readings_per_sec']:.2f} readings/sec")
        
        self.results['varying_intervals'] = results
        return results
    
    def benchmark_encryption_scalability(self, 
                                        batch_sizes: List[int],
                                        num_meters: int = 10) -> Dict:
        """
        Benchmark encryption performance with different batch sizes
        Tests how batch size affects encryption overhead
        
        Args:
            batch_sizes: List of batch sizes to test
            num_meters: Number of meters
        
        Returns:
            Dictionary with benchmark results
        """
        self.logger.log_message(f"=== Benchmark: Encryption Scalability ===")
        
        results = {}
        
        for batch_size in batch_sizes:
            self.logger.log_message(f"Testing batch size {batch_size} with {num_meters} meters...")
            
            # Create server
            server = AnalyticsServer()
            server.start()
            
            # Create meter grid
            meter_config = self.config.get('smart_meters', {})
            server_config = self.config.get('analytics_server', {})
            
            grid = SmartMeterGrid(
                num_meters=num_meters,
                server_host=server_config.get('host', '127.0.0.1'),
                server_port=server_config.get('port', 5000),
                reading_interval_sec=meter_config.get('reading_interval_sec', 5),
                batch_size=batch_size
            )
            
            grid.start()
            
            # Run test for 30 seconds
            time.sleep(30)
            
            # Collect metrics
            grid_stats = grid.get_grid_statistics()
            
            test_result = {
                'batch_size': batch_size,
                'num_meters': num_meters,
                'grid_stats': grid_stats,
                'avg_encryption_time_ms': grid_stats.get('avg_encryption_time_ms', 0),
                'total_batches': sum(len(m.encryption_times) for m in grid.meters)
            }
            
            results[f"batch_{batch_size}"] = test_result
            
            # Log metric
            self.logger.log_metric({
                'test': 'encryption_scalability',
                'batch_size': batch_size,
                'num_meters': num_meters,
                'avg_encryption_time_ms': test_result['avg_encryption_time_ms']
            })
            
            # Cleanup
            grid.stop()
            server.stop()
            
            self.logger.log_message(f"✓ Batch size {batch_size}: "
                                   f"avg encryption time {test_result['avg_encryption_time_ms']:.2f}ms")
        
        self.results['encryption_scalability'] = results
        return results
    
    def generate_report(self) -> str:
        """
        Generate performance analysis report
        
        Returns:
            Formatted report string
        """
        report = []
        report.append("\n" + "="*60)
        report.append("SMART GRID SYSTEM PERFORMANCE REPORT")
        report.append("="*60)
        
        # Summary
        report.append("\nSUMMARY")
        report.append("-" * 60)
        report.append(f"Total tests run: {len(self.results)}")
        
        # Detailed results
        for test_name, test_results in self.results.items():
            report.append(f"\n{test_name.upper()}")
            report.append("-" * 60)
            
            if isinstance(test_results, dict):
                for config_name, result in test_results.items():
                    report.append(f"\nConfiguration: {config_name}")
                    report.append(json.dumps(result, indent=2, default=str))
        
        report.append("\n" + "="*60)
        
        return "\n".join(report)
    
    def run_full_benchmark(self):
        """Run complete benchmark suite"""
        
        self.logger.log_message("Starting full benchmark suite...")
        self.logger.log_message(f"Configuration: {json.dumps(self.config, indent=2)}")
        
        # Test 1: Varying meters
        meter_counts = [2, 5, 10, 20]
        self.benchmark_varying_meters(meter_counts, duration_sec=20)
        
        time.sleep(2)
        
        # Test 2: Varying intervals
        intervals = [1, 5, 10, 30]
        self.benchmark_varying_intervals(intervals, num_meters=5, duration_sec=20)
        
        time.sleep(2)
        
        # Test 3: Encryption scalability
        batch_sizes = [1, 5, 10, 20]
        self.benchmark_encryption_scalability(batch_sizes, num_meters=5)
        
        # Generate and print report
        report = self.generate_report()
        print(report)
        
        # Save report
        FileManager.write_json(
            f"{self.logger.log_dir}/benchmark_report.json",
            self.results
        )
        
        with open(f"{self.logger.log_dir}/benchmark_report.txt", 'w') as f:
            f.write(report)
        
        self.logger.log_message("Benchmark suite complete!")
        self.logger.log_message(f"Results saved to {self.logger.log_dir}")


def main():
    """Run the performance benchmark"""
    
    benchmark = SystemBenchmark()
    benchmark.run_full_benchmark()


if __name__ == '__main__':
    main()
