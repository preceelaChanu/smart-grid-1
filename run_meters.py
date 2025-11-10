"""
Standalone smart meter simulator
Run this to start only the meter clients (without server)
"""

import sys
import os
import time
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from common.utils import ConfigurationManager
from smart_meters.meter_client import SmartMeterGrid


def main():
    parser = argparse.ArgumentParser(description='Smart Meter Client Simulator')
    parser.add_argument('--meters', type=int, default=5, help='Number of meters')
    parser.add_argument('--interval', type=float, default=5, help='Reading interval (seconds)')
    parser.add_argument('--duration', type=int, default=60, help='Duration to run (seconds)')
    parser.add_argument('--host', default='127.0.0.1', help='Server host')
    parser.add_argument('--port', type=int, default=5000, help='Server port')
    
    args = parser.parse_args()
    
    print(f"Starting {args.meters} smart meters...")
    print(f"  Interval: {args.interval}s")
    print(f"  Server: {args.host}:{args.port}")
    print(f"  Duration: {args.duration}s\n")
    
    grid = SmartMeterGrid(
        num_meters=args.meters,
        server_host=args.host,
        server_port=args.port,
        reading_interval_sec=args.interval
    )
    
    grid.start()
    
    try:
        time.sleep(args.duration)
    except KeyboardInterrupt:
        print("\n\nInterrupt received")
    
    grid.stop()
    
    # Print statistics
    stats = grid.get_grid_statistics()
    print("\n=== Final Statistics ===")
    print(f"Total meters: {stats['num_meters']}")
    print(f"Total readings: {stats['total_readings']}")
    print(f"Avg encryption time: {stats['avg_encryption_time_ms']:.2f}ms")
    print(f"Throughput: {stats['throughput_readings_per_sec']:.2f} readings/sec")


if __name__ == '__main__':
    main()
