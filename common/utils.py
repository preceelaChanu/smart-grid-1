"""
Common utilities for the smart grid system
Includes encryption, communication, and data structures
"""

import json
import time
import os
from dataclasses import dataclass, asdict
from typing import List, Dict, Any, Tuple
from datetime import datetime
import struct


@dataclass
class MeterReading:
    """Data structure for a single meter reading"""
    meter_id: int
    timestamp: float
    power_reading: float
    reading_id: int
    
    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)
    
    def to_json(self) -> str:
        return json.dumps(self.to_dict())
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'MeterReading':
        return cls(**data)


@dataclass
class EncryptedData:
    """Data structure for encrypted readings"""
    meter_id: int
    timestamp: float
    ciphertext: bytes  # Serialized encrypted data
    encryption_time_ms: float
    scheme: str = "CKKS"
    encrypted_count: int = 1  # Number of readings in this ciphertext
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            'meter_id': self.meter_id,
            'timestamp': self.timestamp,
            'ciphertext': self.ciphertext.hex() if isinstance(self.ciphertext, bytes) else self.ciphertext,
            'encryption_time_ms': self.encryption_time_ms,
            'scheme': self.scheme,
            'encrypted_count': self.encrypted_count
        }
    
    def to_json(self) -> str:
        return json.dumps(self.to_dict())
    
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'EncryptedData':
        ciphertext = bytes.fromhex(data['ciphertext']) if isinstance(data['ciphertext'], str) else data['ciphertext']
        return cls(
            meter_id=data['meter_id'],
            timestamp=data['timestamp'],
            ciphertext=ciphertext,
            encryption_time_ms=data['encryption_time_ms'],
            scheme=data.get('scheme', 'CKKS'),
            encrypted_count=data.get('encrypted_count', 1)
        )


@dataclass
class AnalyticsResult:
    """Data structure for analytics results"""
    timestamp: float
    operation: str  # 'sum', 'mean', 'max', 'min', 'variance'
    encrypted_result: bytes
    computation_time_ms: float
    num_meters: int
    num_readings: int
    
    def to_dict(self) -> Dict[str, Any]:
        return {
            'timestamp': self.timestamp,
            'operation': self.operation,
            'encrypted_result': self.encrypted_result.hex() if isinstance(self.encrypted_result, bytes) else self.encrypted_result,
            'computation_time_ms': self.computation_time_ms,
            'num_meters': self.num_meters,
            'num_readings': self.num_readings
        }
    
    def to_json(self) -> str:
        return json.dumps(self.to_dict())


@dataclass
class PerformanceMetric:
    """Data structure for performance metrics"""
    timestamp: float
    num_meters: int
    num_readings: int
    total_encryption_time_ms: float
    avg_encryption_time_ms: float
    max_encryption_time_ms: float
    min_encryption_time_ms: float
    total_communication_time_ms: float
    total_computation_time_ms: float
    avg_computation_time_ms: float
    memory_usage_mb: float
    throughput_readings_per_sec: float
    
    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)
    
    def to_json(self) -> str:
        return json.dumps(self.to_dict())


class ConfigurationManager:
    """Manages configuration from YAML files"""
    
    @staticmethod
    def load_config(config_path: str) -> Dict[str, Any]:
        """Load configuration from YAML file"""
        try:
            import yaml
            with open(config_path, 'r') as f:
                config = yaml.safe_load(f)
            return config
        except ImportError:
            # Fallback if PyYAML not available
            return ConfigurationManager._load_yaml_fallback(config_path)
    
    @staticmethod
    def _load_yaml_fallback(config_path: str) -> Dict[str, Any]:
        """Simple YAML parser for basic structures"""
        config = {}
        current_section = None
        
        with open(config_path, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                
                if ':' in line and not line.startswith('  '):
                    current_section = line.split(':')[0].strip()
                    config[current_section] = {}
                elif ':' in line and current_section:
                    key, value = line.split(':', 1)
                    key = key.strip()
                    value = value.strip()
                    
                    # Try to convert to appropriate type
                    if value.lower() in ['true', 'false']:
                        value = value.lower() == 'true'
                    elif value.isdigit():
                        value = int(value)
                    elif value.replace('.', '', 1).isdigit():
                        value = float(value)
                    elif value.startswith('"') and value.endswith('"'):
                        value = value[1:-1]
                    
                    config[current_section][key] = value
        
        return config
    
    @staticmethod
    def get_default_config() -> Dict[str, Any]:
        """Return default configuration values"""
        return {
            'ckks': {
                'poly_modulus_degree': 8192,
                'coeff_modulus': 40,
                'scale': 40,
                'scheme': 'CKKS'
            },
            'smart_meters': {
                'num_meters': 10,
                'reading_interval_sec': 5,
                'max_power_reading': 10000,
                'noise_level': 0.1,
                'batch_size': 5
            },
            'analytics_server': {
                'host': '127.0.0.1',
                'port': 5000,
                'max_connections': 20,
                'buffer_size': 1024
            },
            'logger': {
                'log_level': 'INFO',
                'log_file': './logs/system.log'
            }
        }


class TimerContext:
    """Context manager for timing operations"""
    
    def __init__(self, operation_name: str = "Operation"):
        self.operation_name = operation_name
        self.start_time = None
        self.elapsed_ms = 0
    
    def __enter__(self):
        self.start_time = time.perf_counter()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.elapsed_ms = (time.perf_counter() - self.start_time) * 1000
    
    def __str__(self):
        return f"{self.operation_name}: {self.elapsed_ms:.2f}ms"


class FileManager:
    """Manages file operations for logs and results"""
    
    @staticmethod
    def ensure_directory(path: str):
        """Create directory if it doesn't exist"""
        os.makedirs(path, exist_ok=True)
    
    @staticmethod
    def write_json(filepath: str, data: Any):
        """Write data to JSON file"""
        FileManager.ensure_directory(os.path.dirname(filepath))
        with open(filepath, 'w') as f:
            json.dump(data, f, indent=2)
    
    @staticmethod
    def append_json_line(filepath: str, data: Any):
        """Append JSON line to file (JSONL format)"""
        FileManager.ensure_directory(os.path.dirname(filepath))
        with open(filepath, 'a') as f:
            f.write(json.dumps(data) + '\n')
    
    @staticmethod
    def read_json(filepath: str) -> Any:
        """Read JSON file"""
        with open(filepath, 'r') as f:
            return json.load(f)
    
    @staticmethod
    def append_log(filepath: str, message: str):
        """Append message to log file"""
        FileManager.ensure_directory(os.path.dirname(filepath))
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
        with open(filepath, 'a') as f:
            f.write(f"[{timestamp}] {message}\n")


class DataGenerator:
    """Generates realistic smart meter readings"""
    
    @staticmethod
    def generate_power_reading(
        base_load: float = 1000,
        variance: float = 200,
        periodic_amplitude: float = 500
    ) -> float:
        """
        Generate a realistic power reading with base load, variance, and periodic component
        
        Args:
            base_load: Base power consumption in watts
            variance: Random variance in watts
            periodic_amplitude: Amplitude of periodic variation (daily pattern)
        
        Returns:
            Power reading in watts
        """
        import random
        import math
        
        time_factor = (time.time() % 86400) / 86400  # Fraction of day
        periodic_component = periodic_amplitude * math.sin(2 * math.pi * time_factor)
        random_component = random.uniform(-variance, variance)
        
        reading = base_load + periodic_component + random_component
        return max(0, reading)  # Ensure non-negative
    
    @staticmethod
    def generate_meter_readings(
        meter_id: int,
        num_readings: int,
        interval_sec: float = 1
    ) -> List[MeterReading]:
        """
        Generate a sequence of meter readings
        
        Args:
            meter_id: Identifier for the meter
            num_readings: Number of readings to generate
            interval_sec: Time interval between readings
        
        Returns:
            List of MeterReading objects
        """
        readings = []
        start_time = time.time()
        
        for i in range(num_readings):
            timestamp = start_time + (i * interval_sec)
            power = DataGenerator.generate_power_reading()
            
            reading = MeterReading(
                meter_id=meter_id,
                timestamp=timestamp,
                power_reading=power,
                reading_id=i
            )
            readings.append(reading)
        
        return readings
