"""
Standalone analytics server
Run this to start only the server (meters can connect to it)
"""

import sys
import os
import time
import argparse

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from analytics_server.server import AnalyticsServer


def main():
    parser = argparse.ArgumentParser(description='Analytics Server')
    parser.add_argument('--host', default='127.0.0.1', help='Server host')
    parser.add_argument('--port', type=int, default=5000, help='Server port')
    parser.add_argument('--duration', type=int, default=0, help='Duration to run (0 = infinite)')
    
    args = parser.parse_args()
    
    print(f"Starting Analytics Server...")
    print(f"  Host: {args.host}")
    print(f"  Port: {args.port}\n")
    
    server = AnalyticsServer(host=args.host, port=args.port)
    server.start()
    
    try:
        if args.duration > 0:
            print(f"Running for {args.duration} seconds (Ctrl+C to stop)...")
            time.sleep(args.duration)
        else:
            print("Server running (Ctrl+C to stop)...")
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        print("\n\nInterrupt received")
    
    server.stop()
    
    # Print statistics
    stats = server.get_server_statistics()
    print("\n=== Final Statistics ===")
    print(f"Active meters: {stats['active_meters']}")
    print(f"Total readings received: {stats['total_readings_received']}")
    print(f"Total bytes received: {stats['total_bytes_received']}")


if __name__ == '__main__':
    main()
