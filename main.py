"""
Main system orchestrator
Coordinates smart meters, analytics server, and performance logging
"""

import sys
import os
import time
import json
import argparse
from typing import Optional

# Add current directory to path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from config import ConfigurationManager
from smart_meters.meter_client import SmartMeterGrid
from analytics_server.server import AnalyticsServer
from logger.performance_logger import PerformanceLogger, SystemBenchmark
from common.utils import FileManager


class SmartGridSystem:
    """
    Main smart grid system orchestrator
    Manages all components: meters, server, and logging
    """
    
    def __init__(self, config_path: str = './config/system_config.yaml'):
        """
        Initialize smart grid system
        
        Args:
            config_path: Path to configuration file
        """
        self.config = ConfigurationManager.load_config(config_path)
        self.logger = PerformanceLogger()
        
        self.meter_grid = None
        self.analytics_server = None
        self.is_running = False
    
    def start_system(self):
        """Start the complete smart grid system"""
        
        self.logger.log_message("Starting Smart Grid System...")
        
        server_config = self.config.get('analytics_server', {})
        meter_config = self.config.get('smart_meters', {})
        
        # Start analytics server
        self.logger.log_message("Initializing Analytics Server...")
        self.analytics_server = AnalyticsServer(
            host=server_config.get('host', '127.0.0.1'),
            port=server_config.get('port', 5000),
            max_connections=server_config.get('max_connections', 20)
        )
        self.analytics_server.start()
        
        time.sleep(1)  # Allow server to start
        
        # Start meter grid
        self.logger.log_message("Initializing Smart Meter Grid...")
        self.meter_grid = SmartMeterGrid(
            num_meters=meter_config.get('num_meters', 10),
            server_host=server_config.get('host', '127.0.0.1'),
            server_port=server_config.get('port', 5000),
            reading_interval_sec=meter_config.get('reading_interval_sec', 5),
            batch_size=meter_config.get('batch_size', 5)
        )
        self.meter_grid.start()
        
        self.is_running = True
        self.logger.log_message("✓ Smart Grid System started successfully")
    
    def stop_system(self):
        """Stop the complete smart grid system"""
        
        self.logger.log_message("Stopping Smart Grid System...")
        
        if self.meter_grid:
            self.meter_grid.stop()
        
        if self.analytics_server:
            self.analytics_server.stop()
        
        self.is_running = False
        self.logger.log_message("✓ Smart Grid System stopped")
    
    def run_analytics(self):
        """Perform analytics on collected encrypted data"""
        
        if not self.analytics_server:
            self.logger.log_message("No analytics server available", level="WARNING")
            return
        
        self.logger.log_message("Performing HE Analytics...")
        
        # Compute various aggregations
        result_sum = self.analytics_server.compute_aggregate_sum()
        if result_sum:
            encrypted_sum, comp_time, num_readings = result_sum
            self.logger.log_metric({
                'operation': 'encrypted_sum',
                'num_readings': num_readings,
                'computation_time_ms': comp_time
            })
        
        result_mean = self.analytics_server.compute_aggregate_mean()
        if result_mean:
            encrypted_mean, comp_time, num_readings = result_mean
            self.logger.log_metric({
                'operation': 'encrypted_mean',
                'num_readings': num_readings,
                'computation_time_ms': comp_time
            })
        
        self.logger.log_message("✓ Analytics complete")
    
    def print_statistics(self):
        """Print current system statistics"""
        
        print("\n" + "="*60)
        print("SMART GRID SYSTEM STATISTICS")
        print("="*60)
        
        if self.meter_grid:
            grid_stats = self.meter_grid.get_grid_statistics()
            print("\nMeter Grid:")
            print(json.dumps(grid_stats, indent=2))
        
        if self.analytics_server:
            server_stats = self.analytics_server.get_server_statistics()
            print("\nAnalytics Server:")
            print(json.dumps(server_stats, indent=2))
        
        print("="*60 + "\n")
    
    def save_results(self, output_dir: str = './results'):
        """Save all system results"""
        
        if self.analytics_server:
            self.analytics_server.save_results(output_dir)
        
        self.logger.log_message(f"Results saved to {output_dir}")


def main():
    """Main entry point for smart grid system"""
    
    parser = argparse.ArgumentParser(
        description='Smart Grid System with Homomorphic Encryption'
    )
    
    parser.add_argument(
        '--mode',
        choices=['run', 'benchmark', 'demo'],
        default='demo',
        help='Operation mode: run system, run benchmark, or demo'
    )
    
    parser.add_argument(
        '--duration',
        type=int,
        default=30,
        help='Duration to run system (seconds)'
    )
    
    parser.add_argument(
        '--config',
        default='./config/system_config.yaml',
        help='Path to configuration file'
    )
    
    parser.add_argument(
        '--output',
        default='./results',
        help='Output directory for results'
    )
    
    args = parser.parse_args()
    
    try:
        if args.mode == 'benchmark':
            # Run full benchmark suite
            print("Running benchmark suite...")
            benchmark = SystemBenchmark(args.config)
            benchmark.run_full_benchmark()
            
        elif args.mode == 'run':
            # Run system for specified duration
            system = SmartGridSystem(args.config)
            system.start_system()
            
            try:
                print(f"\nSystem running for {args.duration} seconds...")
                time.sleep(args.duration)
            except KeyboardInterrupt:
                print("\nInterrupt received")
            
            # Perform analytics
            system.run_analytics()
            system.print_statistics()
            system.save_results(args.output)
            system.stop_system()
            
        else:  # demo mode
            # Short demo run
            print("Running system demo (30 seconds)...")
            system = SmartGridSystem(args.config)
            system.start_system()
            
            try:
                time.sleep(30)
            except KeyboardInterrupt:
                print("\nInterrupt received")
            
            system.run_analytics()
            system.print_statistics()
            system.save_results(args.output)
            system.stop_system()
    
    except KeyboardInterrupt:
        print("\n\nSystem interrupted by user")
        sys.exit(0)
    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
