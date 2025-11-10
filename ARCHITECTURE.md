# Smart Grid System with Homomorphic Encryption - Architecture Guide

## Overview

This smart grid system implements a real-world IoT scenario where smart meters send encrypted power consumption readings to an analytics server. The system uses **Levelled Homomorphic Encryption via CKKS** to enable analytics on encrypted data without decryption.

## System Architecture

### Components

```
┌─────────────────────────────────────────────────────────────┐
│                     Smart Grid System                        │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────────┐      ┌─────────────────────────┐  │
│  │  Smart Meters (N)    │      │  Analytics Server       │  │
│  │  ┌────────────────┐  │      │  ┌─────────────────────┐│  │
│  │  │ Meter 0        │  │      │  │ Encrypted Data Store││  │
│  │  ├────────────────┤  │      │  │ ┌─────────────────┐ ││  │
│  │  │ Meter 1        │──┼──┐   │  │ │ CKKS Operations│ ││  │
│  │  ├────────────────┤  │  │   │  │ │ - Sum          │ ││  │
│  │  │ Meter 2        │  │  │   │  │ │ - Mean         │ ││  │
│  │  ├────────────────┤  │  ├──→│  │ │ - Max/Min      │ ││  │
│  │  │ ...            │  │  │   │  │ │ - Variance     │ ││  │
│  │  │ Meter N        │  │  │   │  │ └─────────────────┘ ││  │
│  │  └────────────────┘  │  │   │  └─────────────────────┘│  │
│  └──────────────────────┘  │   └─────────────────────────┘  │
│                           │                                   │
│                    TCP/IP Network                             │
│                  (Encrypted Data)                             │
│                                                               │
└─────────────────────────────────────────────────────────────┘
         │
         │ Performance Metrics
         ↓
┌─────────────────────────────────────────────────────────────┐
│           Performance Logger & Benchmarking                  │
│                                                               │
│  • Encryption time vs meter count                            │
│  • Communication overhead analysis                           │
│  • Computation time per operation                            │
│  • Memory usage tracking                                     │
│  • Throughput measurement                                    │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

## Directory Structure

```
smart-grid/
├── config/
│   └── system_config.yaml          # All configuration parameters
├── common/
│   ├── __init__.py
│   ├── utils.py                    # Shared utilities and data structures
│   └── encryption.py               # CKKS encryption engine
├── smart_meters/
│   ├── __init__.py
│   └── meter_client.py             # Smart meter client logic
├── analytics_server/
│   ├── __init__.py
│   └── server.py                   # Analytics server logic
├── logger/
│   ├── __init__.py
│   └── performance_logger.py       # Performance tracking and benchmarking
├── main.py                         # Main orchestrator
├── run_meters.py                   # Standalone meter client script
├── run_server.py                   # Standalone server script
├── run_benchmark.py                # Benchmark runner
└── README.md                       # Project documentation
```

## Key Design Principles

### 1. Configuration Separation
- All configurable parameters are in `config/system_config.yaml`
- Logic code uses ConfigurationManager to load settings
- No hardcoded values in business logic

### 2. Modular Components
- **Smart Meters**: Independent clients with threading for reading and sending
- **Analytics Server**: Stateless computation on encrypted data
- **Logger**: Tracks performance metrics independently
- **Common Module**: Shared utilities and encryption engine

### 3. Real-World Simulation
- Meters generate realistic power readings with:
  - Base load
  - Periodic variation (daily patterns)
  - Random noise
- Configurable reading intervals
- Batching of readings before encryption (reduces encryption overhead)
- Network simulation with timeouts

### 4. Homomorphic Encryption Features
- **CKKS Scheme**: Levelled homomorphic encryption for approximate arithmetic
- **Batch Operations**: Encrypt multiple readings in single operation
- **Secure Aggregation**: Sum/mean computed without decryption
- **Polynomial Operations**: Support for complex computations

## Module Details

### common/utils.py
Data structures and utilities:
- `MeterReading`: Single power reading
- `EncryptedData`: Encrypted readings packet
- `AnalyticsResult`: Result from HE operations
- `PerformanceMetric`: Performance tracking
- `ConfigurationManager`: YAML config loading
- `TimerContext`: Performance timing context manager
- `DataGenerator`: Realistic data generation

### common/encryption.py
Homomorphic encryption:
- `CKKSEncryptionEngine`: CKKS encryption/decryption
- `EncryptedAggregator`: HE aggregation operations
- `KeyManager`: Encryption key management

### smart_meters/meter_client.py
Smart meter simulation:
- `SmartMeter`: Individual meter client
  - Reading thread: Continuously collects power readings
  - Sending thread: Encrypts and sends batched readings
  - Statistics tracking
- `SmartMeterGrid`: Manages multiple meters
  - Shared encryption engine
  - Aggregated statistics

### analytics_server/server.py
Analytics computation:
- `AnalyticsServer`: Main server
  - Multi-threaded client handling
  - Encrypted data storage
  - Homomorphic aggregation
  - Result persistence
- Operations: sum, mean, max, min, variance

### logger/performance_logger.py
Performance analysis:
- `PerformanceLogger`: Log management
- `SystemBenchmark`: Benchmark suite
  - Test varying meter counts (scaling test)
  - Test varying intervals (interval test)
  - Test batch size effects (encryption test)
  - Report generation

## Configuration File

All system parameters are in `config/system_config.yaml`:

```yaml
# CKKS Parameters
ckks:
  poly_modulus_degree: 8192    # Higher = more precision, slower
  coeff_modulus: 40             # Bit length for moduli
  scale: 40                     # Scaling for CKKS

# Smart Meter Configuration
smart_meters:
  num_meters: 10                # Number of meters
  reading_interval_sec: 5       # How often meters collect
  batch_size: 5                 # Readings per encryption

# Server Configuration
analytics_server:
  host: "127.0.0.1"
  port: 5000
  max_connections: 20

# Performance Testing
performance_intervals:
  short_interval: 1
  medium_interval: 30
  long_interval: 300
```

## Running the System

### Option 1: Complete System (Integrated)
```bash
python main.py --mode run --duration 60
```

### Option 2: Run Components Separately

**Terminal 1 - Analytics Server:**
```bash
python run_server.py --host 127.0.0.1 --port 5000
```

**Terminal 2 - Smart Meters:**
```bash
python run_meters.py --meters 10 --interval 5 --duration 60
```

### Option 3: Run Benchmarks
```bash
# Full benchmark suite
python run_benchmark.py

# Quick benchmark (shorter durations)
python run_benchmark.py --quick
```

### Option 4: Demo Mode
```bash
python main.py --mode demo
```

## Performance Metrics Tracked

### Encryption Metrics
- Encryption time per batch
- Average encryption time per reading
- Encryption time distribution

### Communication Metrics
- Network transmission time
- Message sizes
- Throughput (readings/second)

### Computation Metrics
- HE operation times (sum, mean, etc.)
- Computation depth handling
- Result generation time

### Scalability Metrics
- Performance vs number of meters
- Performance vs reading interval
- Performance vs batch size

## Benchmark Scenarios

### 1. Varying Meter Counts
Tests how encryption scales with more meters:
- 2 meters → 5 → 10 → 20 meters
- Fixed interval: 5 seconds
- Measures throughput and encryption time

### 2. Varying Data Intervals
Tests performance with different data rates:
- 1s → 5s → 10s → 30s intervals
- Fixed 5 meters
- Measures readings per second

### 3. Encryption Scalability
Tests batch size effects:
- Batch sizes: 1 → 5 → 10 → 20
- 10 meters
- Measures encryption time per batch

## Output Files

After running, results are saved to `./results/`:
- `analytics_results.json` - HE computation results
- `performance_metrics.json` - Performance data
- `server_stats.json` - Server statistics
- Logs in `./logs/`

## Security Considerations

1. **Encryption**: CKKS provides semantic security
2. **Key Management**: Context serialization for key sharing
3. **No Decryption at Server**: Analytics done on ciphertexts
4. **Batching Benefits**: Reduces encryption overhead

## Performance Characteristics

- CKKS encryption: ~50-500ms per batch (depends on batch size)
- Network latency: ~10-100ms (simulated)
- HE aggregation: ~10-50ms per operation
- Throughput: 100-1000 readings/second (depends on config)

## Future Enhancements

- [ ] Approximate HE max/min operations
- [ ] Full variance computation on encrypted data
- [ ] Persistent database for results
- [ ] Real network simulation
- [ ] Authentication and integrity verification
- [ ] Secure multi-party computation integration
