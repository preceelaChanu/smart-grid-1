# Smart Grid System with Homomorphic Encryption

A real-world privacy-preserving smart grid system using **Levelled Homomorphic Encryption (CKKS)** for secure analytics without decryption.

## Overview

This system demonstrates a realistic IoT scenario where:
- **Smart Meters (Clients)** collect power consumption readings and encrypt them using CKKS homomorphic encryption
- **Analytics Server** performs aggregation and analysis on encrypted data without ever seeing plaintext readings
- **Performance Logger** tracks system behavior under various configurations

The system is fully modular with configuration separated from logic, allowing easy experimentation with different parameters.

## Key Features

✅ **Real-World Simulation**
- Realistic power consumption patterns with daily periodicity
- Multiple meters operating concurrently
- Configurable reading intervals and batch sizes
- Network simulation with latency

✅ **Homomorphic Encryption**
- CKKS levelled HE scheme via TensorSEAL
- Batch encryption of multiple readings
- Secure aggregation (sum, mean, etc.) on encrypted data
- No decryption required at server

✅ **Modular Architecture**
- Separate configuration file (`config/system_config.yaml`)
- Independent components: meters, server, logger
- Reusable encryption engine
- Clean separation of concerns

✅ **Comprehensive Performance Analysis**
- Encryption time tracking
- Communication overhead measurement
- Computation cost analysis
- Scalability testing (meters, intervals, batch sizes)
- Detailed benchmark reports

## System Architecture

```
Smart Meters                Analytics Server           Performance Logger
     ↓                              ↓                           ↓
Collect readings     →  Encrypt with CKKS  →  Send encrypted data  →  Track metrics
Generate patterns       Batch operations        Perform HE analytics      Benchmark
                       Store encrypted data     Aggregate results
```

## Quick Start

### Installation

```bash
# Clone repository
cd smart-grid

# Install dependencies
pip install -r requirements.txt
```

### Run Full System (30 second demo)
```bash
python main.py --mode demo
```

### Run Components Separately

**Terminal 1 - Start server:**
```bash
python run_server.py
```

**Terminal 2 - Start meters:**
```bash
python run_meters.py --meters 10 --interval 5
```

### Run Benchmarks
```bash
# Full benchmark (2-3 minutes)
python run_benchmark.py

# Quick benchmark (30 seconds)
python run_benchmark.py --quick
```

## Configuration

All parameters are in `config/system_config.yaml`:

```yaml
ckks:
  poly_modulus_degree: 8192  # Encryption precision
  coeff_modulus: 40          # Security level
  scale: 40                  # Scaling factor

smart_meters:
  num_meters: 10             # Number of simulated meters
  reading_interval_sec: 5    # How often to read
  batch_size: 5              # Readings per encryption

analytics_server:
  host: "127.0.0.1"
  port: 5000
  max_connections: 20
```

Modify this file to change system behavior without editing code!

## Project Structure

```
smart-grid/
├── config/
│   └── system_config.yaml           # System configuration (EDIT THIS)
├── common/
│   ├── utils.py                     # Shared data structures
│   └── encryption.py                # CKKS encryption engine
├── smart_meters/
│   └── meter_client.py              # Meter simulation logic
├── analytics_server/
│   └── server.py                    # Server logic
├── logger/
│   └── performance_logger.py        # Benchmarking and logging
├── main.py                          # System orchestrator
├── run_*.py                         # Standalone scripts
├── ARCHITECTURE.md                  # Detailed design documentation
└── README.md                        # This file
```

## Running Modes

### 1. Demo Mode (default)
Quick 30-second demonstration:
```bash
python main.py --mode demo
```

### 2. System Mode
Run system for specified duration with analytics:
```bash
python main.py --mode run --duration 120
```

### 3. Benchmark Mode
Run comprehensive performance tests:
```bash
python main.py --mode benchmark
```

### 4. Standalone Components
Run individual components for testing:
```bash
python run_server.py --host 127.0.0.1 --port 5000
python run_meters.py --meters 10 --interval 5 --duration 60
python run_benchmark.py --quick
```

## Performance Benchmarks

The system tracks:

| Metric | Typical Range |
|--------|---------------|
| Encryption Time | 50-500 ms per batch |
| Network Latency | 10-100 ms |
| HE Aggregation | 10-50 ms per operation |
| Throughput | 100-1000 readings/sec |

## Benchmark Scenarios

1. **Varying Meter Counts** (2→5→10→20 meters)
   - Tests scaling with more data sources
   - Measures throughput per meter

2. **Varying Intervals** (1s→5s→10s→30s)
   - Tests performance with different data rates
   - Measures readings per second

3. **Encryption Scalability** (batch size 1→5→10→20)
   - Tests efficiency of batching
   - Measures encryption overhead

## Output

Results are saved to `./results/`:
- `analytics_results.json` - HE computation results
- `performance_metrics.json` - Detailed metrics
- `server_stats.json` - Server statistics
- `logs/system.log` - System events
- `logs/performance.json` - Performance data

## System Design Principles

### Configuration ≠ Code
- All parameters in `config/system_config.yaml`
- Zero hardcoded values in logic
- ConfigurationManager handles loading
- Change config without touching code

### Modular Components
- Smart Meters: Independent clients with threading
- Analytics Server: Stateless HE computation
- Logger: Independent performance tracking
- Common: Shared utilities and encryption

### Real-World Fidelity
- Realistic power consumption patterns
- Concurrent meter operation
- Network simulation
- Configurable behaviors

### Production-Ready Code
- Error handling throughout
- Thread-safe operations
- Resource cleanup
- Comprehensive logging

## Homomorphic Encryption Details

### CKKS Scheme
- Levelled encryption: operations limited by depth
- Batch processing: multiple plaintexts in one ciphertext
- Approximate arithmetic: suitable for power readings
- Security: semantic security under LWE assumption

### Operations Supported
- Addition: sum encrypted vectors
- Scalar multiplication: scale readings
- Reduction: aggregate values
- More complex operations possible with key management

### Performance Characteristics
- Encryption: O(N log N) time for N values
- Aggregation: O(log N) multiplication depth
- Batching: amortizes cost over multiple values

## Future Enhancements

- [ ] Encrypted comparison for max/min operations
- [ ] Full variance computation on encrypted data
- [ ] Persistent database backend
- [ ] Real network simulation with jitter
- [ ] TLS encryption for network communication
- [ ] Secure multi-party computation integration
- [ ] Threshold decryption for privacy

## References

- TensorSEAL: Encrypted Neural Networks
- CKKS Scheme: https://eprint.iacr.org/2016/421
- Homomorphic Encryption: https://en.wikipedia.org/wiki/Homomorphic_encryption

## License

MIT License - See LICENSE file for details

## Contact

For questions or issues, please open an issue on GitHub.
