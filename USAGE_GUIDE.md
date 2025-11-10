# Smart Grid System - Usage Guide

## Table of Contents

1. [Installation](#installation)
2. [Configuration](#configuration)
3. [Running the System](#running-the-system)
4. [Understanding the Output](#understanding-the-output)
5. [Performance Tuning](#performance-tuning)
6. [Troubleshooting](#troubleshooting)
7. [Advanced Usage](#advanced-usage)

## Installation

### Prerequisites
- Python 3.8+
- pip package manager

### Step 1: Clone the Repository
```bash
git clone https://github.com/preceelaChanu/smart-grid.git
cd smart-grid
```

### Step 2: Install Dependencies
```bash
pip install -r requirements.txt
```

This will install:
- TensorSEAL for homomorphic encryption
- PyYAML for configuration
- NumPy for numerical operations
- Pytest for testing

### Step 3: Verify Installation
```bash
python -c "import tenseal; print('TensorSEAL installed:', tenseal.__version__)"
python -c "import yaml; print('PyYAML installed')"
```

## Configuration

All system parameters are in `config/system_config.yaml`. Edit this file to customize behavior.

### CKKS Encryption Parameters

```yaml
ckks:
  poly_modulus_degree: 8192    # Affects precision and performance
  coeff_modulus: 40            # Security parameter (higher = stronger but slower)
  scale: 40                    # Scaling factor for CKKS (affects precision)
```

**How to adjust:**
- **Precision**: Increase `poly_modulus_degree` (8192 → 16384) for more decimal places
- **Security**: Increase `coeff_modulus` (40 → 50) for stronger encryption
- **Speed**: Decrease `poly_modulus_degree` for faster encryption (but less precise)

### Smart Meters Configuration

```yaml
smart_meters:
  num_meters: 10               # Number of simulated meters
  reading_interval_sec: 5      # Time between readings (seconds)
  max_power_reading: 10000     # Maximum power value (watts)
  noise_level: 0.1             # Random noise as percentage
  batch_size: 5                # Readings to batch before encryption
```

**Recommended combinations:**
- **Light load test**: 2-5 meters, 10s interval, batch_size=5
- **Normal load**: 10 meters, 5s interval, batch_size=5
- **Heavy load**: 20+ meters, 1s interval, batch_size=10

### Server Configuration

```yaml
analytics_server:
  host: "127.0.0.1"
  port: 5000
  max_connections: 20
  buffer_size: 1024
```

- Change `port` if 5000 is in use
- Increase `max_connections` for more meters
- Increase `buffer_size` for larger encrypted messages

## Running the System

### Quick Start (Demo)

Run a 30-second demo with default settings:

```bash
python main.py --mode demo
```

Output will show:
- Smart meters collecting readings
- Server receiving encrypted data
- HE analytics computation
- Final statistics

### Full System Test

Run system for a specified duration:

```bash
python main.py --mode run --duration 120 --output ./results
```

This runs for 2 minutes and saves results to `./results/`

### Run Components Separately

For testing or custom configurations:

**Terminal 1 - Start server:**
```bash
python run_server.py --host 127.0.0.1 --port 5000 --duration 300
```

**Terminal 2 - Start meters:**
```bash
python run_meters.py \
  --meters 10 \
  --interval 5 \
  --duration 300 \
  --host 127.0.0.1 \
  --port 5000
```

Server will listen, meters will connect and send encrypted data.

### Performance Benchmarking

Run comprehensive benchmarks:

```bash
# Full benchmark (tests multiple configurations)
python run_benchmark.py

# Quick benchmark (shorter tests)
python run_benchmark.py --quick

# Custom configuration
python run_benchmark.py --config ./config/system_config.yaml
```

Benchmark tests:
1. **Varying Meters**: 2 → 5 → 10 → 20 meters
2. **Varying Intervals**: 1s → 5s → 10s → 30s
3. **Batch Size Effects**: 1 → 5 → 10 → 20 readings/batch

## Understanding the Output

### Console Output

```
[Server] Started on 127.0.0.1:5000
[Meter 0] Started
[Meter 0] Sent encrypted batch (5 readings). Encryption: 125.34ms, Communication: 8.23ms
[Server] Received encrypted batch from meter 0 (5 readings, encrypted in 125.34ms)
```

- **Encryption time**: How long CKKS encryption took
- **Communication time**: Network round-trip time
- **Batch size**: Number of readings encrypted together

### Results Files

**`./results/analytics_results.json`**
```json
[
  {
    "timestamp": 1234567890.123,
    "operation": "sum",
    "encrypted_result": "abcd1234...",
    "computation_time_ms": 45.67,
    "num_meters": 10,
    "num_readings": 50
  }
]
```

**`./results/performance_metrics.json`**
```json
[
  {
    "timestamp": 1234567890.123,
    "num_meters": 10,
    "num_readings": 50,
    "total_encryption_time_ms": 625.00,
    "avg_encryption_time_ms": 125.00,
    "total_computation_time_ms": 45.67,
    "throughput_readings_per_sec": 1.67
  }
]
```

**`./results/server_stats.json`**
```json
{
  "host": "127.0.0.1",
  "port": 5000,
  "active_meters": 10,
  "total_readings_received": 500,
  "total_bytes_received": 125000,
  "analytics_results_computed": 3
}
```

### Log Files

**`./logs/system.log`**
- All system events and timestamps
- Errors and warnings

**`./logs/performance.json`**
- JSONL format (one JSON object per line)
- Detailed performance metrics for each operation

## Performance Tuning

### For Maximum Throughput

```yaml
smart_meters:
  num_meters: 20
  reading_interval_sec: 1      # Fast readings
  batch_size: 20               # Large batches

ckks:
  poly_modulus_degree: 4096    # Faster encryption
  coeff_modulus: 30            # Acceptable security
```

### For Maximum Precision

```yaml
ckks:
  poly_modulus_degree: 16384   # More precision
  coeff_modulus: 50            # Strong security
  scale: 50                    # Fine scaling

smart_meters:
  batch_size: 5                # Smaller batches for latency
```

### For Maximum Security

```yaml
ckks:
  poly_modulus_degree: 16384
  coeff_modulus: 60
  scale: 60

analytics_server:
  max_connections: 10          # Limit connections
```

### For Testing/Development

```yaml
smart_meters:
  num_meters: 2
  reading_interval_sec: 10
  batch_size: 1                # Single reading per batch

ckks:
  poly_modulus_degree: 4096
  coeff_modulus: 30
```

## Troubleshooting

### Port Already in Use

**Error:** `Address already in use: ('127.0.0.1', 5000)`

**Solution:** Use a different port:
```bash
python run_server.py --port 5001
python run_meters.py --port 5001
```

### Connection Refused

**Error:** `Cannot connect to server at 127.0.0.1:5000`

**Cause:** Server not running or wrong host/port

**Solution:**
1. Make sure server is running
2. Check if meters and server use same host/port
3. Try localhost instead of 127.0.0.1

### TensorSEAL Not Found

**Error:** `ModuleNotFoundError: No module named 'tenseal'`

**Solution:**
```bash
pip install --upgrade tenseal
```

### Memory Issues

**Error:** `MemoryError` or system becomes slow

**Solution:** Reduce configuration:
```yaml
smart_meters:
  num_meters: 5           # Reduce meters
  batch_size: 3           # Smaller batches

ckks:
  poly_modulus_degree: 4096  # Smaller degree
```

### Slow Encryption

**Symptoms:** Encryption taking 1000+ ms per batch

**Solutions:**
1. Reduce `poly_modulus_degree` from 8192 to 4096
2. Reduce `batch_size` (encrypt more frequently, smaller batches)
3. Increase `reading_interval_sec` (collect readings less frequently)

## Advanced Usage

### Custom Configuration Files

Create a new config file:

```bash
cp config/system_config.yaml config/custom_test.yaml
# Edit config/custom_test.yaml
python main.py --config config/custom_test.yaml
```

### Running Multiple Servers

For distributed testing:

```bash
# Terminal 1
python run_server.py --port 5000

# Terminal 2
python run_server.py --port 5001

# Terminal 3
python run_meters.py --meters 5 --port 5000

# Terminal 4
python run_meters.py --meters 5 --port 5001
```

### Analyzing Results

Process results with Python:

```python
import json

# Load results
with open('results/performance_metrics.json') as f:
    metrics = json.load(f)

# Calculate averages
encryption_times = [m['avg_encryption_time_ms'] for m in metrics]
avg_time = sum(encryption_times) / len(encryption_times)
print(f"Average encryption time: {avg_time:.2f}ms")

# Find peak throughput
throughputs = [m['throughput_readings_per_sec'] for m in metrics]
max_throughput = max(throughputs)
print(f"Peak throughput: {max_throughput:.2f} readings/sec")
```

### Customizing Meter Behavior

Modify `smart_meters/meter_client.py`:

```python
# Change power generation pattern
power_reading = DataGenerator.generate_power_reading(
    base_load=5000,           # Higher base
    variance=1000,            # More variation
    periodic_amplitude=2000   # Stronger daily pattern
)
```

### Adding New Operations

Add to `analytics_server/server.py`:

```python
def compute_custom_operation(self):
    """Custom HE operation"""
    # Your code here
    pass
```

## Best Practices

1. **Start small**: Test with 2-5 meters before scaling
2. **Monitor resources**: Watch CPU and memory usage
3. **Use separate terminals**: Run server and meters in different terminals
4. **Save results**: Always use `--output` flag to save test results
5. **Document config**: Keep notes on what config produced what results
6. **Clean up**: Delete old log files before new tests

## Next Steps

- Read `ARCHITECTURE.md` for design details
- Check `common/encryption.py` for CKKS implementation
- Explore `logger/performance_logger.py` for benchmarking
- Contribute improvements!
