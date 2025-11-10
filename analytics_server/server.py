"""
Analytics Server Logic
Receives encrypted data from smart meters and performs homomorphic computations
"""

import threading
import socket
import json
import time
from typing import Dict, List, Optional, Tuple
from collections import defaultdict
from dataclasses import asdict
import sys
import os

# Add parent directory to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from common.utils import (
    EncryptedData, AnalyticsResult, PerformanceMetric, TimerContext, 
    ConfigurationManager, FileManager
)
from common.encryption import CKKSEncryptionEngine, EncryptedAggregator


class AnalyticsServer:
    """
    Server that receives encrypted meter readings and performs analytics
    Capabilities:
    - Receive encrypted data from meters
    - Perform homomorphic aggregation (sum, mean, etc.) without decryption
    - Store results and metrics
    - Provide secure analytics without ever seeing plaintext
    """
    
    def __init__(
        self,
        host: str = '127.0.0.1',
        port: int = 5000,
        max_connections: int = 20,
        buffer_size: int = 4096
    ):
        """
        Initialize analytics server
        
        Args:
            host: Server host address
            port: Server port number
            max_connections: Maximum concurrent connections
            buffer_size: Buffer size for network operations
        """
        self.host = host
        self.port = port
        self.max_connections = max_connections
        self.buffer_size = buffer_size
        
        # Encryption setup
        self.encryption_engine = CKKSEncryptionEngine()
        self.aggregator = EncryptedAggregator(self.encryption_engine)
        
        # Data storage
        self.encrypted_readings: Dict[int, List[bytes]] = defaultdict(list)  # meter_id -> encrypted data
        self.reading_timestamps: Dict[int, List[float]] = defaultdict(list)  # meter_id -> timestamps
        self.analytics_results: List[AnalyticsResult] = []
        self.performance_metrics: List[PerformanceMetric] = []
        
        # State management
        self.is_running = False
        self.server_socket = None
        self.server_thread = None
        self.lock = threading.Lock()
        
        # Statistics
        self.total_readings_received = 0
        self.total_bytes_received = 0
        self.active_meters = set()
    
    def start(self):
        """Start the analytics server"""
        if self.is_running:
            return
        
        self.is_running = True
        self.server_thread = threading.Thread(target=self._server_loop, daemon=True)
        self.server_thread.start()
        print(f"[Server] Started on {self.host}:{self.port}")
    
    def stop(self):
        """Stop the analytics server"""
        self.is_running = False
        
        if self.server_socket:
            try:
                self.server_socket.close()
            except:
                pass
        
        if self.server_thread:
            self.server_thread.join(timeout=5)
        
        print(f"[Server] Stopped. Total readings: {self.total_readings_received}")
    
    def _server_loop(self):
        """
        Main server loop: listen for and handle incoming connections
        Runs in separate thread
        """
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(self.max_connections)
            
            print(f"[Server] Listening for connections...")
            
            while self.is_running:
                try:
                    self.server_socket.settimeout(1)
                    client_socket, client_addr = self.server_socket.accept()
                    
                    # Handle client in separate thread
                    client_thread = threading.Thread(
                        target=self._handle_client,
                        args=(client_socket, client_addr),
                        daemon=True
                    )
                    client_thread.start()
                    
                except socket.timeout:
                    continue
                except Exception as e:
                    if self.is_running:
                        print(f"[Server] Accept error: {e}")
                        
        except Exception as e:
            print(f"[Server] Fatal error: {e}")
        finally:
            self.is_running = False
    
    def _handle_client(self, client_socket: socket.socket, client_addr: Tuple[str, int]):
        """
        Handle a client connection
        
        Args:
            client_socket: Connected client socket
            client_addr: Client address tuple (host, port)
        """
        try:
            client_socket.settimeout(5)
            
            # Receive data
            data = b''
            while True:
                chunk = client_socket.recv(self.buffer_size)
                if not chunk:
                    break
                data += chunk
                if b'\n' in data:
                    break
            
            if data:
                self._process_encrypted_data(data.decode('utf-8').strip())
                self.total_bytes_received += len(data)
                
                # Send acknowledgment
                client_socket.sendall(b'ACK')
            
        except socket.timeout:
            print(f"[Server] Timeout from {client_addr}")
        except Exception as e:
            print(f"[Server] Error handling client {client_addr}: {e}")
        finally:
            try:
                client_socket.close()
            except:
                pass
    
    def _process_encrypted_data(self, data_json: str):
        """
        Process received encrypted data
        
        Args:
            data_json: JSON string containing encrypted data
        """
        try:
            data_dict = json.loads(data_json)
            encrypted_data = EncryptedData.from_dict(data_dict)
            
            # Store encrypted data
            with self.lock:
                self.encrypted_readings[encrypted_data.meter_id].append(encrypted_data.ciphertext)
                self.reading_timestamps[encrypted_data.meter_id].append(encrypted_data.timestamp)
                self.total_readings_received += encrypted_data.encrypted_count
                self.active_meters.add(encrypted_data.meter_id)
            
            print(f"[Server] Received encrypted batch from meter {encrypted_data.meter_id} "
                  f"({encrypted_data.encrypted_count} readings, "
                  f"encrypted in {encrypted_data.encryption_time_ms:.2f}ms)")
            
        except Exception as e:
            print(f"[Server] Error processing data: {e}")
    
    def compute_aggregate_sum(self) -> Optional[Tuple[bytes, float, int]]:
        """
        Compute homomorphic sum of all received readings
        
        Returns:
            Tuple of (encrypted_sum, computation_time_ms, num_readings) or None
        """
        with self.lock:
            all_encrypted = []
            total_count = 0
            
            for meter_readings in self.encrypted_readings.values():
                all_encrypted.extend(meter_readings)
                total_count += len(meter_readings)
        
        if not all_encrypted:
            print("[Server] No encrypted data to aggregate")
            return None
        
        try:
            encrypted_sum, comp_time = self.aggregator.sum_encrypted_readings(all_encrypted)
            
            result = AnalyticsResult(
                timestamp=time.time(),
                operation='sum',
                encrypted_result=encrypted_sum,
                computation_time_ms=comp_time,
                num_meters=len(self.active_meters),
                num_readings=total_count
            )
            
            with self.lock:
                self.analytics_results.append(result)
            
            print(f"[Server] Computed encrypted sum of {total_count} readings from {len(self.active_meters)} meters "
                  f"in {comp_time:.2f}ms")
            
            return encrypted_sum, comp_time, total_count
            
        except Exception as e:
            print(f"[Server] Error computing sum: {e}")
            return None
    
    def compute_aggregate_mean(self) -> Optional[Tuple[bytes, float, int]]:
        """
        Compute homomorphic mean of all received readings
        
        Returns:
            Tuple of (encrypted_mean, computation_time_ms, num_readings) or None
        """
        with self.lock:
            all_encrypted = []
            total_count = 0
            
            for meter_readings in self.encrypted_readings.values():
                all_encrypted.extend(meter_readings)
                total_count += len(meter_readings)
        
        if not all_encrypted:
            return None
        
        try:
            encrypted_mean, comp_time = self.aggregator.mean_encrypted_readings(all_encrypted)
            
            result = AnalyticsResult(
                timestamp=time.time(),
                operation='mean',
                encrypted_result=encrypted_mean,
                computation_time_ms=comp_time,
                num_meters=len(self.active_meters),
                num_readings=total_count
            )
            
            with self.lock:
                self.analytics_results.append(result)
            
            print(f"[Server] Computed encrypted mean of {total_count} readings "
                  f"in {comp_time:.2f}ms")
            
            return encrypted_mean, comp_time, total_count
            
        except Exception as e:
            print(f"[Server] Error computing mean: {e}")
            return None
    
    def record_performance_metric(self, num_meters: int, num_readings: int, 
                                 total_encryption_ms: float, total_computation_ms: float,
                                 memory_usage_mb: float):
        """
        Record performance metrics
        
        Args:
            num_meters: Number of active meters
            num_readings: Total readings received
            total_encryption_ms: Total time spent on encryption
            total_computation_ms: Total time spent on computation
            memory_usage_mb: Current memory usage
        """
        metric = PerformanceMetric(
            timestamp=time.time(),
            num_meters=num_meters,
            num_readings=num_readings,
            total_encryption_time_ms=total_encryption_ms,
            avg_encryption_time_ms=total_encryption_ms / num_readings if num_readings > 0 else 0,
            max_encryption_time_ms=0,  # Would track from meters
            min_encryption_time_ms=0,  # Would track from meters
            total_communication_time_ms=0,
            total_computation_time_ms=total_computation_ms,
            avg_computation_time_ms=total_computation_ms / num_meters if num_meters > 0 else 0,
            memory_usage_mb=memory_usage_mb,
            throughput_readings_per_sec=num_readings / (time.time() - time.time() + 1)
        )
        
        with self.lock:
            self.performance_metrics.append(metric)
    
    def get_server_statistics(self) -> dict:
        """Get server statistics"""
        with self.lock:
            return {
                'host': self.host,
                'port': self.port,
                'active_meters': len(self.active_meters),
                'total_readings_received': self.total_readings_received,
                'total_bytes_received': self.total_bytes_received,
                'analytics_results_computed': len(self.analytics_results),
                'performance_metrics_recorded': len(self.performance_metrics)
            }
    
    def save_results(self, output_dir: str = './results'):
        """
        Save all results and metrics to files
        
        Args:
            output_dir: Directory to save results
        """
        FileManager.ensure_directory(output_dir)
        
        # Save analytics results
        results_data = [asdict(r) for r in self.analytics_results]
        FileManager.write_json(f"{output_dir}/analytics_results.json", results_data)
        
        # Save performance metrics
        metrics_data = [asdict(m) for m in self.performance_metrics]
        FileManager.write_json(f"{output_dir}/performance_metrics.json", metrics_data)
        
        # Save server statistics
        stats = self.get_server_statistics()
        FileManager.write_json(f"{output_dir}/server_stats.json", stats)
        
        print(f"[Server] Results saved to {output_dir}")


def main():
    """Demo: Run analytics server"""
    
    # Load configuration
    config = ConfigurationManager.get_default_config()
    server_config = config.get('analytics_server', {})
    
    host = server_config.get('host', '127.0.0.1')
    port = server_config.get('port', 5000)
    
    # Start server
    server = AnalyticsServer(host=host, port=port)
    server.start()
    
    try:
        # Run for 60 seconds
        time.sleep(60)
        
        # Perform analytics
        print("\n=== Performing Analytics ===")
        server.compute_aggregate_sum()
        server.compute_aggregate_mean()
        
    except KeyboardInterrupt:
        print("\nInterrupt received")
    finally:
        server.stop()
        
        # Save results
        server.save_results()
        
        # Print statistics
        stats = server.get_server_statistics()
        print("\n=== Server Statistics ===")
        print(json.dumps(stats, indent=2))


if __name__ == '__main__':
    main()
