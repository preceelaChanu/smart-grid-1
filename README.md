# Privacy-Preserving Smart Grid Analytics Framework

## Overview

This project implements a **real-time, scalable** framework for benchmarking Post-Quantum Homomorphic Encryption (HE) performance in Smart Grid environments. It features a privacy-preserving 3-tier architecture where **configurable numbers of Smart Meters** encrypt real-number energy consumption data using **CKKS encryption**, transmit it to an **Aggregator** for homomorphic processing, and forward results to a **Control Center** for decryption and analytics.

The system utilizes the **CKKS (Cheon-Kim-Kim-Song) scheme** from Microsoft SEAL with **configurable security parameters** including polynomial degree, scale precision, and security levels. The framework supports **accelerated simulation** with configurable timing intervals for rapid research and development.

## Key Features

- **Scalable Architecture**: Configurable number of concurrent smart meters with batched aggregation
- **Real-time Performance**: High-performance encryption with continuous data streaming  
- **Research-Grade Metrics**: Comprehensive CSV performance data for algorithm comparison
- **Post-Quantum Security**: Configurable security levels with certificate-based PKI
- **Accelerated Testing**: Configurable simulation speedup for rapid analysis
- **Flexible Configuration**: All parameters adjustable via configuration files

## System Architecture

The framework implements a **continuous operation model** with real-time encrypted data streaming:

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Smart Meters  │    │    Aggregator    │    │ Control Center  │
│ (configurable)  │───▶│   (Batched)      │───▶│  (Analytics)    │
│                 │    │                  │    │                 │
│ • CKKS Encrypt  │    │ • Homomorphic    │    │ • Decrypt       │
│ • Configurable  │    │   Aggregation    │    │ • Statistics    │
│   intervals     │    │ • Configurable   │    │ • CSV Export    │
│ • Configurable  │    │   connections    │    │ • Performance   │
│   parameters    │    │                  │    │   Analysis      │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

**Data Flow:**
1. **Smart Meters**: Generate synthetic energy data, encrypt with CKKS using configured parameters
2. **Aggregator**: Collect encrypted data using configurable batching, perform homomorphic aggregation
3. **Control Center**: Receive aggregated ciphertext, decrypt, and export performance analytics

## Performance Metrics

The system generates comprehensive research data in CSV format:

| Metric Category | Key Measurements |
|-----------------|------------------|
| **Encryption** | Timing, compression ratio, security level, parameter impact |
| **Network** | Throughput, latency, connection overhead, scalability |
| **Scalability** | Concurrent connections, aggregation time, memory usage |
| **Security** | Certificate validation, key distribution, PKI overhead |
| **Homomorphic** | Operation timing, noise growth, computation depth |

All metrics adapt automatically to the configured parameters, enabling comparative analysis across different security and performance settings.


## Core Components

The system consists of focused, production-ready components:

| Component | Executable | Purpose | Key Features |
|-----------|------------|---------|--------------|
| **Key Generation** | `keygen` | Generate CKKS keys | 8192 poly degree, 128-bit security |
| **Certificate Gen** | `certgen` | PKI certificate creation | TLS-ready certificates for all nodes |
| **Key Distribution** | `kdc` | Secure key distribution | Certificate-based authentication |
| **Smart Meters** | `client_continuous` | Real-time data encryption | Continuous operation, performance logging |
| **Aggregator** | `aggregator_continuous` | Homomorphic computation | Batched connections, scalable processing |
| **Control Center** | `control_center_continuous` | Analytics & decryption | Result processing, CSV export |

## Quick Start

### 1. Build the System
```bash
# Install dependencies
./install_dependencies.sh

# Build all components
mkdir -p build && cd build
cmake .. && make -j4
```

### 2. Run Full System Test
```bash
# Generate keys and certificates for n meters
./run_continuous_test.sh

# For research comparison with different configurations
./run_research_comparison.sh
```

### 3. Performance Analysis
```bash
# View real-time encryption metrics
tail -f build/performance_data/encryption_metrics.csv

# Analyze system scalability
tail -f build/performance_data/scalability_metrics.csv
```

## Project Structure
```
smart-grid-1/
├── CMakeLists.txt              # Build configuration
├── config.json                 # System configuration (security, network, timing)
├── run_continuous_test.sh      # Full system test script
├── run_research_comparison.sh  # Research automation script
├── install_dependencies.sh     # Dependency installation
├── include/
│   ├── performance_metrics.h   # Performance measurement framework
│   ├── network_utils.h         # Network communication utilities
│   ├── kdc_client.h            # Key distribution client
│   └── json.hpp                # JSON parsing library
├── keygen/
│   └── keygen.cpp              # CKKS key generation (8192 poly, 40-bit scale)
├── certgen/
│   └── certgen.cpp             # PKI certificate generation
├── kdc/
│   ├── kdc.cpp                 # Key Distribution Center server
│   └── kdc_client.cpp          # KDC client implementation
├── client/
│   └── client_continuous.cpp   # Smart meter with continuous operation
├── aggregator/
│   └── aggregator_continuous.cpp # Batched homomorphic aggregation
├── control_center/
│   └── control_center_continuous.cpp # Analytics and decryption
├── network/
│   └── network_utils.cpp       # TCP/IP communication framework
├── performance/
│   └── performance_metrics.cpp # CSV performance logging
├── keys/                       # Generated cryptographic keys and certificates
└── build/
    ├── performance_data/       # CSV output files for research analysis
    └── [executables]           # Built binaries
```

## Configuration

All system parameters are configurable through `config.json`. Key categories:

### Security Parameters
```json
{
  "poly_modulus_degree": [8192, 16384, 32768],  // Security level (higher = more secure)
  "ckks_scale_bits": [30, 40, 50],              // Precision for real numbers
  "security_level": [128, 192, 256]             // Target security bits
}
```

### System Scale
```json
{
  "num_clients": 10,                    // Number of smart meters (1-100+)
  "data_collection_interval": 20,       // Seconds between readings
  "simulation_acceleration": 15         // Speedup factor for testing
}
```

### Network Configuration
```json
{
  "aggregator": {
    "max_parallel_connections": 50,     // Concurrent connection limit
    "connect_timeout": 30,              // Connection timeout (seconds)
    "batch_size": 10                    // Meters processed per batch
  }
}
```

### Performance Tuning
```json
{
  "performance_metrics": {
    "enable_csv_export": true,          // Export performance data
    "measurement_precision": "nanoseconds", // Timing precision
    "export_interval": 60               // CSV export frequency
  }
}
```
## Research Applications

This framework is designed for academic research and algorithm comparison:

### Performance Benchmarking
- **Encryption Timing**: Nanosecond-precision measurements for algorithm comparison
- **Scalability Analysis**: Connection handling, memory usage, and throughput metrics
- **Network Overhead**: Communication cost analysis for practical deployment
- **Security Validation**: Certificate-based PKI with post-quantum cryptography

### CSV Data Export
Generated performance data includes:
- `encryption_metrics.csv`: Encryption timing, compression ratios, security levels
- `network_metrics.csv`: Throughput, latency, connection overhead
- `scalability_metrics.csv`: Concurrent connections, memory usage, processing time
- `homomorphic_metrics.csv`: Operation timing, noise growth, computation depth
- `security_metrics.csv`: Certificate validation, authentication overhead

### Algorithm Comparison
The standardized CSV format enables comparison with:
- Other homomorphic encryption schemes (BGV, BFV, TFHE)
- Traditional cryptographic approaches
- Performance optimizations and parameter tuning
- Different network architectures and configurations

## Technical Specifications

- **Encryption**: CKKS with configurable polynomial degree (8192-32768)
- **Security**: Configurable post-quantum security levels (128-256 bits)
- **Performance**: Optimized encryption with parameter-dependent timing
- **Scalability**: Configurable smart meter count with batched processing
- **Throughput**: Adaptive batching with configurable connection limits
- **Acceleration**: Configurable simulation speedup for testing scenarios
- **Flexibility**: All parameters adjustable without code changes

## Dependencies

The system automatically installs dependencies via `./install_dependencies.sh`:
- **Microsoft SEAL 4.1+**: Homomorphic encryption library
- **nlohmann/json**: Configuration file parsing
- **OpenSSL**: Certificate generation and validation
- **CMake 3.16+**: Build system

## Usage

### Automated Testing (Recommended)
```bash
# Full system demonstration with configured parameters
./run_continuous_test.sh

# Research comparison across multiple configurations
./run_research_comparison.sh
```

### Manual Component Testing
```bash
# Generate cryptographic keys
./build/keygen

# Start individual components
./build/kdc                      # Key distribution center
./build/client_continuous 1      # Smart meter (specify ID)
./build/aggregator_continuous    # Homomorphic aggregator
./build/control_center_continuous # Analytics center
```

## Configuration Management

All system parameters are controlled via `config.json` - **no recompilation required**:

### Core Parameters
- **`poly_modulus_degree`**: Security level (8192=fast, 16384=strong, 32768=maximum)
- **`ckks_scale_bits`**: Real number precision (30-60 bits)
- **`num_clients`**: Number of smart meters (1-100+)

### Network & Performance
- **`max_parallel_connections`**: Concurrent connection limit
- **`data_collection_interval`**: Timing between meter readings
- **`simulation_acceleration`**: Speedup factor for testing

### Example Configuration
```json
{
  "poly_modulus_degree": 8192,
  "ckks_scale_bits": 40,
  "num_clients": 10,
  "aggregator": {
    "max_parallel_connections": 50,
    "connect_timeout": 30
  }
}
```

## Research Output

The framework automatically generates CSV files for performance analysis:
- **Encryption metrics**: Timing, security parameters, compression ratios
- **Network metrics**: Throughput, latency, connection overhead  
- **Scalability metrics**: Memory usage, concurrent processing
- **Security metrics**: Certificate validation, authentication overhead

## Contributing & License

This framework is designed for academic research in privacy-preserving smart grid analytics. The configurable parameter system and standardized CSV output enable reproducible research and algorithm comparison across different homomorphic encryption approaches.
Uses Microsoft SEAL (MIT License).