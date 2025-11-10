"""
Smart Meter Client Logic
Simulates physical smart meters that collect readings and send encrypted data to server
"""

import threading
import time
import socket
import json
from typing import Optional, List
from dataclasses import asdict
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from common.utils import MeterReading, EncryptedData, DataGenerator, TimerContext, ConfigurationManager
from common.encryption import CKKSEncryptionEngine


class SmartMeter:
    """
    Represents a single smart meter that:
    1. Periodically collects power consumption readings
    2. Encrypts readings using CKKS homomorphic encryption
    3. Sends encrypted data to analytics server
    """
    
    def __init__(
        self,
        meter_id: int,
        server_host: str,
        server_port: int,
        reading_interval_sec: float = 5,
        batch_size: int = 5,
        max_power: float = 10000,
        encryption_engine: Optional[CKKSEncryptionEngine] = None
    ):
        """
        Initialize a smart meter
        
        Args:
            meter_id: Unique identifier for this meter
            server_host: Server hostname/IP
            server_port: Server port number
            reading_interval_sec: Interval between readings (seconds)
            batch_size: Number of readings to batch before encryption
            max_power: Maximum power reading value
            encryption_engine: CKKS encryption engine (if None, creates new one)
        """
        self.meter_id = meter_id
        self.server_host = server_host
        self.server_port = server_port
        self.reading_interval_sec = reading_interval_sec
        self.batch_size = batch_size
        self.max_power = max_power
        
        # Encryption setup
        self.encryption_engine = encryption_engine or CKKSEncryptionEngine()
        
        # State tracking
        self.is_running = False
        self.reading_count = 0
        self.readings_buffer = []
        self.encryption_times = []
        self.communication_times = []
        
        # Thread management
        self.read_thread = None
        self.send_thread = None
        self.lock = threading.Lock()
    
    def start(self):
        """Start the meter (begin collecting and sending readings)"""
        if self.is_running:
            return
        
        self.is_running = True
        self.read_thread = threading.Thread(target=self._reading_loop, daemon=True)
        self.send_thread = threading.Thread(target=self._sending_loop, daemon=True)
        
        self.read_thread.start()
        self.send_thread.start()
        
        print(f"[Meter {self.meter_id}] Started")
    
    def stop(self):
        """Stop the meter"""
        self.is_running = False
        if self.read_thread:
            self.read_thread.join(timeout=5)
        if self.send_thread:
            self.send_thread.join(timeout=5)
        
        print(f"[Meter {self.meter_id}] Stopped. Total readings: {self.reading_count}")
    
    def _reading_loop(self):
        """
        Main loop: continuously collect power readings
        Runs in separate thread
        """
        while self.is_running:
            try:
                # Generate realistic power reading
                power_reading = DataGenerator.generate_power_reading(
                    base_load=2000 + (self.meter_id * 100),  # Vary by meter
                    variance=500,
                    periodic_amplitude=800
                )
                
                # Create reading record
                reading = MeterReading(
                    meter_id=self.meter_id,
                    timestamp=time.time(),
                    power_reading=power_reading,
                    reading_id=self.reading_count
                )
                
                # Add to buffer
                with self.lock:
                    self.readings_buffer.append(reading)
                
                self.reading_count += 1
                
                # Sleep until next reading
                time.sleep(self.reading_interval_sec)
                
            except Exception as e:
                print(f"[Meter {self.meter_id}] Error in reading loop: {e}")
    
    def _sending_loop(self):
        """
        Main loop: encrypt and send readings when batch is ready
        Runs in separate thread
        """
        while self.is_running:
            try:
                # Check if we have enough readings to send
                with self.lock:
                    if len(self.readings_buffer) >= self.batch_size:
                        readings_to_send = self.readings_buffer[:self.batch_size]
                        self.readings_buffer = self.readings_buffer[self.batch_size:]
                    else:
                        readings_to_send = None
                
                if readings_to_send:
                    self._encrypt_and_send(readings_to_send)
                
                time.sleep(0.1)  # Check frequently
                
            except Exception as e:
                print(f"[Meter {self.meter_id}] Error in sending loop: {e}")
    
    def _encrypt_and_send(self, readings: List[MeterReading]):
        """
        Encrypt batch of readings and send to server
        
        Args:
            readings: List of MeterReading objects to encrypt and send
        """
        try:
            # Extract power values for encryption
            power_values = [r.power_reading for r in readings]
            
            # Encrypt the batch
            ciphertext, encryption_time_ms = self.encryption_engine.encrypt_data(power_values)
            
            # Create encrypted data packet
            encrypted_data = EncryptedData(
                meter_id=self.meter_id,
                timestamp=time.time(),
                ciphertext=ciphertext,
                encryption_time_ms=encryption_time_ms,
                scheme="CKKS",
                encrypted_count=len(readings)
            )
            
            # Send to server
            send_time_ms = self._send_to_server(encrypted_data)
            
            self.encryption_times.append(encryption_time_ms)
            self.communication_times.append(send_time_ms)
            
            print(f"[Meter {self.meter_id}] Sent encrypted batch ({len(readings)} readings). "
                  f"Encryption: {encryption_time_ms:.2f}ms, Communication: {send_time_ms:.2f}ms")
            
        except Exception as e:
            print(f"[Meter {self.meter_id}] Error encrypting/sending: {e}")
    
    def _send_to_server(self, encrypted_data: EncryptedData) -> float:
        """
        Send encrypted data to analytics server
        
        Args:
            encrypted_data: EncryptedData object to send
        
        Returns:
            Communication time in milliseconds
        """
        with TimerContext("Network transmission") as timer:
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(5)
                sock.connect((self.server_host, self.server_port))
                
                # Send JSON packet
                data_json = encrypted_data.to_json()
                sock.sendall(data_json.encode('utf-8') + b'\n')
                
                # Wait for acknowledgment
                ack = sock.recv(1024)
                sock.close()
                
                if ack != b'ACK':
                    print(f"[Meter {self.meter_id}] No acknowledgment from server")
                    
            except ConnectionRefusedError:
                print(f"[Meter {self.meter_id}] Cannot connect to server at {self.server_host}:{self.server_port}")
            except Exception as e:
                print(f"[Meter {self.meter_id}] Network error: {e}")
        
        return timer.elapsed_ms
    
    def get_statistics(self) -> dict:
        """
        Get performance statistics for this meter
        
        Returns:
            Dictionary of performance metrics
        """
        return {
            'meter_id': self.meter_id,
            'total_readings': self.reading_count,
            'avg_encryption_time_ms': sum(self.encryption_times) / len(self.encryption_times) if self.encryption_times else 0,
            'max_encryption_time_ms': max(self.encryption_times) if self.encryption_times else 0,
            'min_encryption_time_ms': min(self.encryption_times) if self.encryption_times else 0,
            'avg_communication_time_ms': sum(self.communication_times) / len(self.communication_times) if self.communication_times else 0,
            'max_communication_time_ms': max(self.communication_times) if self.communication_times else 0,
            'batches_sent': len(self.encryption_times)
        }


class SmartMeterGrid:
    """
    Manages a fleet of smart meters
    Coordinates multiple meters and aggregates their statistics
    """
    
    def __init__(
        self,
        num_meters: int,
        server_host: str,
        server_port: int,
        reading_interval_sec: float = 5,
        batch_size: int = 5
    ):
        """
        Initialize smart meter grid
        
        Args:
            num_meters: Number of meters in the grid
            server_host: Server hostname/IP
            server_port: Server port number
            reading_interval_sec: Reading interval for all meters
            batch_size: Batch size for all meters
        """
        self.num_meters = num_meters
        self.server_host = server_host
        self.server_port = server_port
        self.reading_interval_sec = reading_interval_sec
        self.batch_size = batch_size
        
        # Create shared encryption engine
        self.encryption_engine = CKKSEncryptionEngine()
        
        # Create meters
        self.meters = [
            SmartMeter(
                meter_id=i,
                server_host=server_host,
                server_port=server_port,
                reading_interval_sec=reading_interval_sec,
                batch_size=batch_size,
                encryption_engine=self.encryption_engine
            )
            for i in range(num_meters)
        ]
    
    def start(self):
        """Start all meters"""
        for meter in self.meters:
            meter.start()
        print(f"[Grid] Started {self.num_meters} meters")
    
    def stop(self):
        """Stop all meters"""
        for meter in self.meters:
            meter.stop()
        print("[Grid] All meters stopped")
    
    def get_grid_statistics(self) -> dict:
        """Get aggregated statistics for entire grid"""
        stats = {
            'num_meters': self.num_meters,
            'meters': [m.get_statistics() for m in self.meters]
        }
        
        # Aggregate metrics
        if self.meters:
            all_readings = sum(m.reading_count for m in self.meters)
            all_encryption_times = [t for m in self.meters for t in m.encryption_times]
            
            stats['total_readings'] = all_readings
            stats['avg_encryption_time_ms'] = sum(all_encryption_times) / len(all_encryption_times) if all_encryption_times else 0
            stats['throughput_readings_per_sec'] = all_readings / self.reading_interval_sec if self.reading_interval_sec > 0 else 0
        
        return stats


def main():
    """Demo: Run a small smart meter grid"""
    
    # Load configuration
    config = ConfigurationManager.get_default_config()
    meter_config = config.get('smart_meters', {})
    server_config = config.get('analytics_server', {})
    
    num_meters = meter_config.get('num_meters', 3)
    reading_interval = meter_config.get('reading_interval_sec', 2)
    batch_size = meter_config.get('batch_size', 2)
    
    server_host = server_config.get('host', '127.0.0.1')
    server_port = server_config.get('port', 5000)
    
    # Create and start grid
    grid = SmartMeterGrid(
        num_meters=num_meters,
        server_host=server_host,
        server_port=server_port,
        reading_interval_sec=reading_interval,
        batch_size=batch_size
    )
    
    grid.start()
    
    try:
        # Run for 30 seconds
        time.sleep(30)
    except KeyboardInterrupt:
        print("\nInterrupt received")
    finally:
        grid.stop()
        
        # Print statistics
        stats = grid.get_grid_statistics()
        print("\n=== Grid Statistics ===")
        print(json.dumps(stats, indent=2))


if __name__ == '__main__':
    main()
