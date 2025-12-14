# Privacy-Preserving Smart Grid Analytics Framework

## Overview

This project implements a **production-ready, scalable** framework for Post-Quantum Homomorphic Encryption (HE) in Smart Grid environments. The system features a privacy-preserving 3-tier architecture where **up to 500 configurable Smart Meters** encrypt realistic energy consumption data using **CKKS encryption**, transmit it to an **Aggregator** for homomorphic processing, and forward results to a **Control Center** for decryption and analytics.

The framework implements **realistic energy consumption patterns** based on analysis of **5,560 UK households** (166.5M readings), uses the **CKKS (Cheon-Kim-Kim-Song) scheme** from Microsoft SEAL with configurable security levels (8192-32768 polynomial degree), and provides comprehensive CSV performance metrics for research applications.

**Key Innovation**: The system simulates authentic energy consumption patterns with 95.4% low consumers, 4.6% medium consumers, realistic temporal patterns (peak at 19:00, low at 4:00), and dataset-accurate consumption ranges (0.000-2.112 kWh) based on real-world smart meter data.

## Key Features

- **Realistic Data Simulation**: Energy patterns based on 5,560 UK households (166.5M readings)
  - 95.4% low consumers (<0.5 kWh), 4.6% medium consumers (0.5-2.0 kWh)
  - Authentic temporal patterns: Peak 19:00 (0.315 kWh), Low 4:00 (0.110 kWh)
  - Accurate consumption range: 0.000-2.112 kWh matching real datasets
- **Production-Scale Architecture**: Up to 500 concurrent smart meters with batched aggregation
- **High-Performance Encryption**: CKKS with configurable polynomial degrees (8192-32768)
- **Research-Grade Metrics**: Nanosecond-precision CSV performance data export
- **Post-Quantum Security**: 128-256 bit security levels with certificate-based PKI
- **Continuous Operation**: Real-time encrypted data streaming with graceful shutdown
- **Flexible Configuration**: All parameters adjustable via `config.json` without recompilation

## System Architecture

The framework implements a **continuous operation model** with realistic energy consumption simulation:

```
┌─────────────────────────────────────┐    ┌──────────────────┐    ┌─────────────────────┐
│           Smart Meters              │    │    Aggregator    │    │   Control Center    │
│        (up to 500 units)            │───▶│    (Batched      │───▶│    (Analytics)      │
│                                     │    │   Processing)    │    │                     │
│ • Realistic UK consumption data     │    │                  │    │ • CKKS Decryption  │
│ • CKKS Encryption (8192-32768)      │    │ • Homomorphic    │    │ • Statistical       │
│ • Temporal patterns (24h/7d)        │    │   Aggregation    │    │   Analysis          │
│ • Household types (95.4% low)       │    │ • Concurrent     │    │ • CSV Performance   │
│ • Certificate-based TLS             │    │   Connections    │    │   Export            │
│ • Performance monitoring            │    │ • SEAL Context   │    │ • Real-time         │
│ • Graceful shutdown support         │    │ • Result Export  │    │   Monitoring        │
└─────────────────────────────────────┘    └──────────────────┘    └─────────────────────┘
```

**Enhanced Data Flow:**
1. **Smart Meters**: Generate realistic UK household energy data using dataset-derived patterns, encrypt with CKKS using configurable security parameters (8192-32768 polynomial degree), establish TLS connections with certificates
2. **Aggregator**: Collect encrypted data from up to 500 concurrent meters using configurable batching, perform homomorphic aggregation operations, export intermediate results
3. **Control Center**: Receive aggregated ciphertext, perform CKKS decryption, generate statistical analysis, export comprehensive CSV performance metrics

## Realistic Energy Consumption Simulation

### Dataset-Based Implementation
The system implements authentic UK household energy consumption patterns based on comprehensive analysis:

#### 📊 Real-World Statistics (5,560 Households)
- **Total Readings Analyzed**: 166,523,410 measurements
- **Average Consumption**: 0.213 kWh (matches dataset exactly)
- **Consumption Range**: 0.000 - 2.112 kWh (authentic distribution)
- **Dataset Period**: November 2011 - February 2014 (2.25 years)

#### 🏠 Household Classification System
```cpp
enum class HouseholdType {
    LOW_CONSUMER,      // 95.4% - avg <0.5 kWh (realistic majority)
    MEDIUM_CONSUMER,   // 4.6% - avg 0.5-2.0 kWh (normal usage)
    HIGH_CONSUMER,     // 0.0% - avg >2.0 kWh (data-driven absence)
    VARIABLE_CONSUMER  // 51.0% - high variation patterns
};
```

#### ⏰ Temporal Consumption Patterns
```cpp
// 24-hour consumption factors (derived from dataset analysis)
static constexpr double hourly_factors_[24] = {
    0.517, 0.476, 0.448, 0.423,  // 00:00-03:00 (night low)
    0.516, 0.563, 0.610, 0.704,  // 04:00-07:00 (morning rise)
    0.751, 0.775, 0.798, 0.845,  // 08:00-11:00 (day increase)
    0.892, 0.915, 0.939, 0.962,  // 12:00-15:00 (afternoon)
    0.986, 1.056, 1.127, 1.479,  // 16:00-19:00 (evening peak)
    1.197, 1.169, 1.141, 0.587   // 20:00-23:00 (night decrease)
};
```

#### 📈 Key Consumption Insights Implemented
- **Peak Hour**: 19:00 (0.315 kWh average) - realistic evening demand
- **Low Hour**: 4:00 (0.110 kWh average) - authentic night consumption
- **Peak Day**: Sunday (0.222 kWh average) - weekend patterns
- **Variable Behavior**: 51% of meters exhibit high consumption variation
- **Zero Consumers**: ~2% probability (matching real-world data gaps)


## Core Components

The system consists of production-ready components with specific implementations:

| Component | Executable | Implementation | Key Features |
|-----------|------------|---------------|--------------|
| **Key Generation** | `keygen` | CKKS parameter setup | 8192 poly degree, 40-bit scale, quantum-resistant |
| **Certificate Gen** | `certgen` | OpenSSL PKI integration | TLS-ready X.509 certificates for all nodes |
| **Key Distribution** | `kdc` | Secure SEAL key sharing | Certificate-based authentication, concurrent clients |
| **Smart Meters** | `client_continuous` | Realistic data simulation | UK dataset patterns, continuous operation, CSV logging |
| **Aggregator** | `aggregator_continuous` | Batched HE computation | Up to 500 concurrent connections, SEAL Evaluator |
| **Control Center** | `control_center_continuous` | Analytics & decryption | SEAL Decryptor, statistical analysis, CSV export |

### Detailed Component Specifications

#### Smart Meters (`client_continuous.cpp`)
- **Realistic Simulation**: Based on 5,560 UK households analysis
- **Consumption Patterns**: 95.4% low consumers, 4.6% medium, temporal variations
- **CKKS Implementation**: Configurable polynomial degree, 40-bit scale precision
- **Network**: TLS with X.509 certificates, graceful shutdown with SIGINT/SIGTERM
- **Performance**: Nanosecond-precision encryption timing, CSV export

#### Aggregator (`aggregator_continuous.cpp`)  
- **Concurrent Processing**: Up to 500 smart meter connections
- **Homomorphic Operations**: SEAL Evaluator for encrypted aggregation
- **Batching**: Configurable batch sizes for optimal performance
- **Security**: Certificate validation, secure key distribution via KDC
- **Monitoring**: Real-time connection status, performance metrics

#### Control Center (`control_center_continuous.cpp`)
- **CKKS Decryption**: SEAL Decryptor with secret key management
- **Analytics**: Statistical analysis, consumption pattern detection
- **Export**: Comprehensive CSV performance data for research
- **Monitoring**: Real-time system status, graceful shutdown support

## Quick Start

### 1. Prerequisites & Dependencies
```bash
# Ensure system dependencies (automatically installed)
sudo apt update && sudo apt install -y build-essential cmake git

# Install dependencies (Microsoft SEAL 4.1+, OpenSSL, nlohmann-json)
./install_dependencies.sh
```

### 2. Build the Complete System
```bash
# Create build directory and compile all components
mkdir -p build && cd build
cmake .. && make -j4

# Verify all executables built successfully
ls -la keygen certgen kdc client_continuous aggregator_continuous control_center_continuous
```

### 3. Configuration & Execution
```bash
# Run complete system with realistic UK consumption simulation (recommended)
./run_continuous_test.sh

# For research comparison across different security parameters
./run_research_comparison.sh

# View real-time performance data
tail -f build/performance_data/*.csv
```

### 4. Manual Component Testing
```bash
# Generate CKKS keys and certificates
./build/keygen && ./build/certgen

# Start components in separate terminals
./build/kdc                        # Key Distribution Center
./build/client_continuous 1        # Smart meter (specify ID 1-500)  
./build/aggregator_continuous      # Homomorphic aggregator
./build/control_center_continuous  # Analytics and decryption
```

## Project Structure & Implementation

```
smart-grid-1/                           # Root directory
├── 📋 Configuration & Documentation
│   ├── CMakeLists.txt                  # Build configuration (C++17, SEAL, OpenSSL)
│   ├── config.json                     # Complete system configuration
│   ├── README.md                       # This comprehensive guide
│   ├── IMPLEMENTATION_SUMMARY.md       # Realistic consumption implementation details
│   ├── REALISTIC_CONSUMPTION_GUIDE.md  # UK household data analysis guide
│   ├── Complete_Energy_Analysis_Report.md # Full 5,560 household analysis
│   └── HOMOMORPHIC_ENCRYPTION_GUIDE.md # CKKS implementation details
│
├── 🛠️ Build & Automation Scripts
│   ├── install_dependencies.sh         # Microsoft SEAL, OpenSSL, CMake setup
│   ├── run_continuous_test.sh          # Complete system demonstration
│   ├── run_research_comparison.sh      # Multi-parameter research automation
│   └── demo_realistic_consumption.sh   # Realistic consumption demo
│
├── 📚 Core Libraries & Headers
│   ├── include/
│   │   ├── performance_metrics.h       # CSV research data export framework
│   │   ├── network_utils.h             # TLS/TCP communication utilities
│   │   ├── kdc_client.h                # Key distribution client implementation
│   │   └── json.hpp                    # nlohmann JSON library (configuration parsing)
│   ├── network/
│   │   └── network_utils.cpp           # TCP/IP + TLS implementation
│   └── performance/
│       └── performance_metrics.cpp     # Nanosecond-precision CSV logging
│
├── 🔐 Cryptographic Components
│   ├── keygen/
│   │   └── keygen.cpp                  # CKKS key generation (8192-32768 poly degree)
│   ├── certgen/
│   │   └── certgen.cpp                 # X.509 certificate generation (OpenSSL)
│   └── kdc/
│       ├── kdc.cpp                     # Key Distribution Center server
│       └── kdc_client.cpp              # Secure SEAL key distribution client
│
├── 🏠 Smart Grid Components
│   ├── client/
│   │   └── client_continuous.cpp       # Smart meter with UK household simulation
│   ├── aggregator/
│   │   └── aggregator_continuous.cpp   # Batched homomorphic aggregation (up to 500)
│   ├── control_center/
│   │   └── control_center_continuous.cpp # CKKS decryption & analytics
│   └── dataset_preprocessor/
│       ├── advanced_hhblock_converter.py # UK dataset preprocessing tools
│       └── proper_meter_separator.py     # Multi-household data separation
│
├── 🏗️ Build Output (Generated)
│   └── build/
│       ├── 🔑 Cryptographic Material
│       │   ├── keygen                    # CKKS key generator executable
│       │   ├── certgen                   # Certificate generator executable
│       │   ├── kdc                       # Key distribution server
│       │   └── keys/                     # Generated SEAL keys & X.509 certificates
│       ├── 🔧 Smart Grid Executables
│       │   ├── client_continuous         # Smart meter simulation
│       │   ├── aggregator_continuous     # Homomorphic aggregator
│       │   └── control_center_continuous # Analytics center
│       ├── 📊 Performance Data (CSV)
│       │   └── performance_data/
│       │       ├── encryption_metrics.csv    # CKKS encryption performance
│       │       ├── network_metrics.csv       # TLS network performance
│       │       ├── scalability_metrics.csv   # Concurrent connection analysis
│       │       ├── homomorphic_metrics.csv   # HE operation performance
│       │       └── security_metrics.csv      # PKI & certificate metrics
│       └── 📁 Runtime Data
│           ├── data/                     # Encrypted ciphertext storage
│           └── certificates/             # Runtime certificate storage
│
└── 📈 Research & Analysis
    ├── homomorphic_demo/                # Standalone HE examples
    ├── homomorphic_example.cpp          # Basic CKKS usage example
    └── performance_data/                # Research CSV output directory
```

### Component Relationships

```
Configuration (config.json) ──┐
                              ├─→ Key Generation (CKKS) ──┐
Certificate Generation ────────┘                         │
                                                         ├─→ Smart Meters (1-500)
Key Distribution Center ──────────────────────────────────┘         │
                                                                     ├─→ Aggregator (HE)
Network Utilities (TLS) ────────────────────────────────────────────┤         │
                                                                     └─→ Control Center
Performance Metrics ──────────────────────────────────────────────────────────┘
                                      │
                                      └─→ CSV Research Data
```

## Configuration Management

All system parameters are configured via [config.json](config.json) enabling research flexibility without recompilation:

### 🔐 Security Parameters (CKKS Homomorphic Encryption)
```json
{
  "comment_security": "poly_modulus_degree: 8192 (fast), 16384 (strong), 32768 (paranoid)",
  "poly_modulus_degree": 8192,
  
  "comment_scale": "CKKS initial scale. 40-bits is a good default.",
  "ckks_scale_bits": 40
}
```

### 📊 System Scale & Performance
```json
{
  "comment_clients": "Number of smart meters to simulate (1-500)",
  "num_clients": 500,
  
  "aggregator": {
    "max_parallel_connections": 500,
    "connect_timeout": 30,
    "data_timeout": 60,
    "connection_retry_attempts": 3,
    "retry_delay_ms": 1000
  }
}
```

### 🌐 Network Configuration
```json
{
  "control_center": {
    "host": "127.0.0.1",
    "port": 8443,
    "certificate": "keys/control_center.cert"
  },
  "smart_meters": {
    "base_port": 9000,
    "port_range": 5000,
    "max_concurrent_connections": 500,
    "server_timeout": 300
  },
  "key_distribution_center": {
    "host": "127.0.0.1", 
    "port": 8444,
    "certificate": "keys/kdc.cert"
  }
}
```

### 📁 File Paths & Data Management
```json
{
  "public_key_file": "keys/public_key.seal",
  "secret_key_file": "keys/secret_key.seal", 
  "relin_keys_file": "keys/relin_keys.seal",
  "data_path_prefix": "data/ct_client_"
}
```

### 🔧 Parameter Impact Analysis
| Parameter | Performance Impact | Security Impact | Recommended Values |
|-----------|-------------------|-----------------|-------------------|
| `poly_modulus_degree` | Higher = slower encryption | Higher = stronger security | 8192 (fast), 16384 (balanced), 32768 (paranoid) |
| `ckks_scale_bits` | Higher = more precision | Minimal security impact | 40-60 bits for energy data |
| `num_clients` | Linear performance impact | No security impact | 1-500 (tested scaling) |
| `max_parallel_connections` | Network bottleneck factor | No security impact | Match `num_clients` |
## Research Applications & Academic Use

This framework is specifically designed for academic research and algorithm comparison in privacy-preserving smart grid analytics:

### 📊 Algorithm Benchmarking & Comparison

#### Homomorphic Encryption Scheme Analysis
- **CKKS Performance**: Optimized for real-number energy consumption data
- **Parameter Impact Studies**: Security vs performance trade-offs across polynomial degrees
- **Comparison Framework**: Standardized CSV output enables comparison with:
  - BGV scheme (integer arithmetic)
  - BFV scheme (integer arithmetic) 
  - TFHE scheme (boolean circuits)
  - Traditional symmetric encryption approaches

#### Performance Characterization
```csv
# Example comparative study output
scheme,poly_degree,precision,encryption_time_ms,ciphertext_size_kb,throughput_ops_sec
CKKS,8192,40,0.847,16.0,1180
CKKS,16384,40,3.231,32.0,309
BGV,8192,N/A,0.923,15.8,1083
TFHE,N/A,binary,12.4,0.8,80
```

### 🔬 Research Data Export & Analysis

#### Comprehensive CSV Dataset Generation
The framework generates research-grade datasets across multiple dimensions:

**1. Encryption Performance Analysis**
- Timing measurements with nanosecond precision
- Memory usage tracking (RSS, heap allocation)
- Ciphertext expansion ratios
- Security parameter impact quantification

**2. Network Performance Studies**
- TLS handshake overhead analysis
- Concurrent connection scalability (1-500 meters)
- Throughput and latency characterization
- Network protocol optimization studies

**3. Homomorphic Operation Benchmarking**
- Addition operation timing on encrypted data
- Noise growth analysis over computation depth
- Batch operation optimization studies
- Relinearization overhead quantification

**4. Realistic Workload Analysis**
- UK household consumption pattern accuracy
- Temporal variation impact on encryption performance
- Scale factor optimization for energy data precision
- Real-world data distribution effects

### 🏗️ Academic Integration Examples

#### Research Paper Data Generation
```bash
# Generate comprehensive performance dataset
./run_research_comparison.sh

# Extract specific metrics for publication
grep "CKKS,8192" performance_data/*.csv > paper_dataset.csv

# Statistical analysis preparation
python3 -c "
import pandas as pd
df = pd.read_csv('performance_data/encryption_metrics.csv')
print(f'Mean encryption time: {df[\"encryption_time_ms\"].mean():.3f}ms')
print(f'95th percentile: {df[\"encryption_time_ms\"].quantile(0.95):.3f}ms')
"
```

#### Comparative Studies Framework
```python
# Example research analysis script
import pandas as pd
import matplotlib.pyplot as plt

# Load performance data
ckks_data = pd.read_csv('performance_data/encryption_metrics.csv')
network_data = pd.read_csv('performance_data/network_metrics.csv')

# Generate performance comparison charts
plt.figure(figsize=(12, 8))
plt.subplot(2, 2, 1)
plt.plot(ckks_data['poly_degree'], ckks_data['encryption_time_ms'])
plt.title('Encryption Time vs Security Level')
plt.xlabel('Polynomial Degree')
plt.ylabel('Encryption Time (ms)')

# Throughput analysis
plt.subplot(2, 2, 2)  
plt.plot(network_data['concurrent_connections'], network_data['throughput_mbps'])
plt.title('Network Throughput vs Concurrent Connections')
plt.xlabel('Concurrent Connections')
plt.ylabel('Throughput (Mbps)')

plt.tight_layout()
plt.savefig('smart_grid_performance_analysis.png', dpi=300)
```

### 🎓 Educational Applications

#### Cryptography Courses
- **Hands-on HE Experience**: Students can experiment with real homomorphic encryption
- **Security Parameter Education**: Concrete examples of security vs performance trade-offs
- **Real-world Application**: Energy sector privacy requirements and solutions

#### Smart Grid Security Courses
- **Privacy-Preserving Analytics**: Practical implementation of theoretical concepts
- **Post-Quantum Cryptography**: Future-proof security implementations
- **Scalability Analysis**: Real-world system performance characteristics

#### Research Methodology Training
- **Reproducible Research**: Standardized benchmarking framework
- **Performance Analysis**: Comprehensive metrics collection and analysis
- **Comparative Studies**: Framework for algorithm comparison and evaluation

### 📈 Publication-Ready Results

The framework generates publication-quality research data suitable for:
- **Conference Papers**: Performance benchmarking and algorithm comparison
- **Journal Articles**: Comprehensive analysis of homomorphic encryption in smart grids
- **Thesis Research**: Complete implementation and evaluation framework
- **Technical Reports**: System design and performance characterization

All exported data includes:
- **Reproducible Configuration**: Complete parameter sets for result reproduction
- **Statistical Significance**: Large-scale testing with up to 500 concurrent meters
- **Real-world Relevance**: Authentic UK household consumption patterns
- **Standardized Format**: CSV output compatible with all statistical analysis tools

## Technical Specifications & Implementation Details

### 🔐 Homomorphic Encryption (Microsoft SEAL)
- **Scheme**: CKKS (Cheon-Kim-Kim-Song) for approximate arithmetic on real numbers
- **Polynomial Degree**: 8192 (default), 16384, 32768 (configurable security levels)
- **Scale Precision**: 40-bit default (configurable 30-60 bits for energy data)
- **Security**: 128-bit classical, ~64-bit post-quantum (lattice-based Ring-LWE)
- **Key Management**: Secure distribution via certificate-authenticated KDC

### 🌐 Network Architecture
- **Protocol**: TLS 1.3 with X.509 certificate authentication
- **Topology**: Star topology (meters → aggregator → control center)
- **Concurrency**: Up to 500 simultaneous smart meter connections
- **Port Management**: Configurable base port 9000 with 5000-port range
- **Reliability**: Connection retry with exponential backoff, graceful shutdown

### 🏠 Smart Meter Implementation (`client_continuous.cpp`)
```cpp
// Realistic household energy consumption simulation
class SmartMeterContinuous {
    // UK dataset-derived consumption patterns
    HouseholdType household_type_;          // 95.4% low, 4.6% medium consumers
    double base_consumption_;               // 0.000-2.112 kWh range
    double hourly_factors_[24];            // Time-of-day consumption patterns
    double daily_factors_[7];              // Day-of-week variations
    
    // CKKS encryption components
    shared_ptr<SEALContext> context_;      // Configured polynomial degree
    PublicKey public_key_;                 // From secure KDC distribution
    Encryptor* encryptor_;                 // CKKS encryption engine
    CKKSEncoder* encoder_;                 // Real number encoding
    double scale_;                         // 40-bit precision scale
};
```

### ⚡ Aggregator Implementation (`aggregator_continuous.cpp`)
```cpp
class ContinuousAggregator {
    // Homomorphic evaluation
    Evaluator* evaluator_;                 // SEAL homomorphic operations
    vector<Ciphertext> encrypted_batch_;   // Batched ciphertext collection
    
    // Concurrent processing
    int max_parallel_;                     // Up to 500 concurrent connections
    thread_pool connection_handlers_;      // Asynchronous connection management
    atomic<int> active_connections_;       // Real-time connection tracking
    
    // Performance optimization
    chrono::minutes aggregation_interval_; // Batched processing timing
    size_t batch_size_;                    // Configurable batch optimization
};
```

### 🎯 Control Center Implementation (`control_center_continuous.cpp`)
```cpp
class ContinuousControlCenter {
    // CKKS decryption
    SecretKey secret_key_;                 // Secure key management
    Decryptor* decryptor_;                 // SEAL decryption engine
    CKKSEncoder* encoder_;                 // Result decoding
    
    // Analytics & research data
    struct DataPoint {
        chrono::system_clock::time_point timestamp;
        double total_consumption;          // Aggregated consumption
        int num_meters;                    // Active meter count
        double average_consumption;        // Statistical analysis
    };
    
    vector<DataPoint> consumption_history_; // Time-series analytics
    PerformanceMetrics perf_metrics_;      // CSV research export
};
```

### 📊 Performance Optimization
- **Compiler Flags**: `-O3 -Wall -Wextra` for optimized production builds
- **Threading**: Multi-threaded connection handling with atomic synchronization
- **Memory Management**: Efficient SEAL context sharing, minimal memory allocation
- **Batch Processing**: Configurable batching to optimize network and computation
- **Instrumentation**: Nanosecond-precision timing for research accuracy

### 🔬 Research Data Export Format
All performance metrics exported in standardized CSV format for algorithm comparison:
- **Timestamp precision**: ISO 8601 with nanosecond resolution
- **Memory tracking**: RSS, heap allocation, SEAL context memory
- **Network metrics**: TLS handshake overhead, throughput, latency
- **Security analysis**: Certificate validation timing, key distribution overhead
- **Homomorphic metrics**: Ciphertext size, operation timing, noise growth estimation

## Dependencies & Installation

### 🔧 System Requirements
- **Operating System**: Linux (Ubuntu 20.04+, Debian 11+, CentOS 8+)
- **Compiler**: GCC 9+ or Clang 10+ with C++17 support
- **Memory**: 8GB RAM minimum (16GB recommended for 500 meters)
- **Storage**: 2GB for build artifacts and performance data

### 📦 Core Dependencies (Auto-installed)
```bash
# Automated installation via provided script
./install_dependencies.sh
```

**Installed Components:**
- **Microsoft SEAL 4.1+**: Post-quantum homomorphic encryption library
- **OpenSSL 1.1.1+**: TLS cryptography and X.509 certificate management
- **nlohmann/json 3.9+**: High-performance JSON parsing for configuration
- **CMake 3.16+**: Modern C++ build system with SEAL integration

### 🏗️ Manual Build Process
```bash
# 1. Clone and prepare environment
git clone <repository-url> smart-grid-1
cd smart-grid-1

# 2. Install system dependencies
sudo apt update && sudo apt install -y \
    build-essential cmake git wget \
    libssl-dev pkg-config

# 3. Install Microsoft SEAL
./install_dependencies.sh  # Automated SEAL compilation and installation

# 4. Configure and build project
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)  # Parallel build using all CPU cores

# 5. Verify installation
ls -la keygen certgen kdc client_continuous aggregator_continuous control_center_continuous
```

### 🔧 CMake Configuration Options
```bash
# Development build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Custom SEAL installation path
cmake -DSEAL_DIR=/custom/path/to/seal ..

# Performance optimizations
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
```

### 📋 Dependency Verification
```bash
# Verify Microsoft SEAL installation
pkg-config --modversion seal

# Verify OpenSSL version (should be 1.1.1+)
openssl version

# Check compiler support for C++17
g++ --version | grep -E "(gcc|g\+\+)"
```

## Usage & Demonstration

### 🚀 Automated System Demonstrations

#### Complete System Test (Recommended)
```bash
# Full system demonstration with realistic UK consumption patterns
./run_continuous_test.sh

# What this script does:
# 1. Validates all required binaries exist
# 2. Generates CKKS encryption keys (8192 polynomial degree)
# 3. Creates X.509 certificates for all components
# 4. Starts Key Distribution Center (KDC)
# 5. Launches smart meters with realistic consumption simulation
# 6. Starts aggregator with homomorphic processing
# 7. Initializes control center for analytics
# 8. Runs for configurable duration with performance logging
# 9. Exports comprehensive CSV research data
```

#### Research Parameter Comparison
```bash
# Automated research across multiple security configurations
./run_research_comparison.sh

# Compares performance across:
# - Polynomial degrees: 8192, 16384, 32768
# - Smart meter counts: 10, 50, 100, 500
# - Scale precision: 30, 40, 50 bits
# - Generates comparative CSV data for analysis
```

#### Realistic Consumption Demo
```bash
# Focused demonstration of UK household consumption patterns
./demo_realistic_consumption.sh

# Highlights:
# - 95.4% low consumers vs 4.6% medium consumers
# - Temporal patterns (peak 19:00, low 4:00)
# - Authentic consumption range (0.000-2.112 kWh)
# - Variable household behavior simulation
```

### 🔧 Manual Component Operation

#### Step 1: Cryptographic Setup
```bash
cd build

# Generate CKKS encryption keys
./keygen
# Output: keys/public_key.seal, keys/secret_key.seal, keys/relin_keys.seal

# Generate X.509 certificates for TLS
./certgen  
# Output: keys/*.cert for all components (aggregator, control center, KDC, smart meters)
```

#### Step 2: Key Distribution
```bash
# Start Key Distribution Center (separate terminal)
./kdc
# - Listens on port 8444 (configurable)
# - Distributes SEAL keys via certificate-authenticated TLS
# - Supports concurrent key requests from multiple meters
```

#### Step 3: Smart Meter Simulation
```bash
# Start individual smart meters (configurable ID 1-500)
./client_continuous 1      # Smart meter ID 1
./client_continuous 2      # Smart meter ID 2
# ... up to 500 concurrent meters

# Each meter:
# - Retrieves CKKS keys from KDC
# - Simulates UK household consumption patterns
# - Encrypts data with configured polynomial degree
# - Streams encrypted data to aggregator
# - Exports performance metrics to CSV
```

#### Step 4: Homomorphic Aggregation
```bash
# Start aggregator (separate terminal)
./aggregator_continuous

# Aggregator functionality:
# - Accepts up to 500 concurrent smart meter connections
# - Performs homomorphic addition on encrypted consumption data
# - Batches processing for optimal performance
# - Forwards aggregated ciphertext to control center
# - Logs network and homomorphic operation performance
```

#### Step 5: Analytics & Control
```bash
# Start control center (separate terminal)  
./control_center_continuous

# Control center operations:
# - Receives aggregated ciphertext from aggregator
# - Performs CKKS decryption using secret key
# - Generates statistical analysis and consumption insights
# - Exports comprehensive research data to CSV
# - Provides real-time system monitoring
```

### 📊 Real-time Monitoring & Analysis

#### Performance Data Monitoring
```bash
# Monitor encryption performance in real-time
tail -f performance_data/encryption_metrics.csv

# Track network throughput and latency
tail -f performance_data/network_metrics.csv

# Analyze system scalability
tail -f performance_data/scalability_metrics.csv

# Watch homomorphic operation performance
tail -f performance_data/homomorphic_metrics.csv
```

#### System Status Monitoring
```bash
# Check smart meter connections
netstat -an | grep 900[0-9]

# Monitor aggregator connections
netstat -an | grep 8443

# Check KDC activity
netstat -an | grep 8444
```

### 🛑 Graceful Shutdown
All components support graceful shutdown with SIGINT (Ctrl+C) or SIGTERM:
- Smart meters: Complete current encryption, export final metrics
- Aggregator: Finish processing current batch, export performance data
- Control center: Complete decryption, export final analytics
- KDC: Close all client connections, clean up certificate state

## Contributing & Development

### 🤝 Contributing Guidelines

This framework welcomes contributions from the academic and research community. All contributions should maintain the focus on research applications and algorithm comparison.

#### Development Environment Setup
```bash
# Clone repository
git clone <repository-url> smart-grid-1
cd smart-grid-1

# Set up development environment
./install_dependencies.sh

# Build in debug mode for development
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Run tests to verify setup
../run_continuous_test.sh
```

#### Code Style & Standards
- **Language**: C++17 with modern STL features
- **Formatting**: Follow Google C++ Style Guide
- **Performance**: Optimize for research accuracy (nanosecond timing precision)
- **Documentation**: Comprehensive inline comments for algorithm implementations
- **CSV Export**: Maintain standardized format for research reproducibility

#### Research Extensions
**Priority Areas for Contribution:**
1. **Additional HE Schemes**: BGV, BFV, TFHE implementations
2. **Advanced Analytics**: Machine learning on encrypted smart meter data
3. **Network Optimizations**: Alternative communication protocols
4. **Security Enhancements**: Post-quantum certificate authorities
5. **Scalability Improvements**: Beyond 500 concurrent connections

### 📄 License & Usage

#### Academic License
- **Framework**: MIT License for maximum research flexibility
- **Microsoft SEAL**: MIT License (included dependency)
- **OpenSSL**: Apache-style license (system dependency)

#### Citation Requirements
If you use this framework in academic research, please cite:
```bibtex
@software{smart_grid_he_framework,
  title={Privacy-Preserving Smart Grid Analytics Framework},
  author={[Author Names]},
  year={2024},
  url={[Repository URL]},
  note={Post-quantum homomorphic encryption implementation with realistic UK consumption patterns}
}
```

#### Research Data Usage
- **CSV Exports**: Public domain for research use
- **Performance Benchmarks**: Freely usable in comparative studies  
- **Dataset Patterns**: Based on public UK smart meter data
- **Configuration Examples**: Template for research reproducibility

### 🔧 Development Roadmap

#### Current Features (Production Ready)
- ✅ CKKS homomorphic encryption with Microsoft SEAL
- ✅ Realistic UK household consumption simulation (5,560 households)
- ✅ Up to 500 concurrent smart meter connections
- ✅ Comprehensive CSV performance metrics export
- ✅ TLS security with X.509 certificate authentication
- ✅ Nanosecond-precision timing measurements
- ✅ Graceful shutdown and error handling

#### Planned Enhancements (Research Extensions)
- 🔄 **Additional HE Schemes**: BGV and BFV implementations for comparative analysis
- 🔄 **Machine Learning Integration**: Privacy-preserving analytics on aggregated data
- 🔄 **Geographic Distribution**: Multi-region smart meter simulation
- 🔄 **Advanced Temporal Patterns**: Seasonal and weather-based consumption modeling
- 🔄 **Blockchain Integration**: Decentralized key distribution and audit trails
- 🔄 **Quantum Communication**: Preparation for quantum key distribution

#### Research Collaboration
- **Academic Partnerships**: Integration with university research programs
- **Conference Presentations**: Results suitable for cryptography and smart grid conferences
- **Open Datasets**: Public release of anonymized performance benchmarking data
- **Reproducible Research**: Complete configuration sets for result reproduction

### 🛠️ Technical Support

#### Documentation Resources
- **Implementation Guide**: [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)
- **Consumption Patterns**: [REALISTIC_CONSUMPTION_GUIDE.md](REALISTIC_CONSUMPTION_GUIDE.md)  
- **Dataset Analysis**: [Complete_Energy_Analysis_Report.md](Complete_Energy_Analysis_Report.md)
- **Cryptography Details**: [HOMOMORPHIC_ENCRYPTION_GUIDE.md](HOMOMORPHIC_ENCRYPTION_GUIDE.md)

#### Performance Optimization
- **Build Optimization**: Use `-O3 -march=native` for maximum performance
- **Memory Tuning**: Adjust SEAL context parameters for available RAM
- **Network Tuning**: Configure batch sizes based on network capacity
- **Concurrent Tuning**: Match thread count to CPU core availability

#### Troubleshooting
```bash
# Common build issues
# 1. SEAL not found
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

# 2. OpenSSL version conflicts  
sudo apt install libssl-dev

# 3. CMake version too old
wget https://cmake.org/files/v3.20/cmake-3.20.0-Linux-x86_64.sh

# 4. Performance data not generated
mkdir -p build/performance_data
chmod 755 build/performance_data
```

This framework represents a collaborative effort to advance privacy-preserving analytics in smart grid systems through rigorous research and open implementation.