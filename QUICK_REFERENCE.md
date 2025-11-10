# Smart Grid System - Quick Reference Guide

## Installation (2 minutes)

```bash
cd smart-grid
pip install -r requirements.txt
python validate_system.py
```

## Quick Start (Choose One)

### Option 1: Demo (30 seconds)
```bash
python main.py --mode demo
```

### Option 2: Full Test (2 minutes)
```bash
python main.py --mode run --duration 120
```

### Option 3: Benchmarks (3-5 minutes)
```bash
python run_benchmark.py --quick
```

### Option 4: Components Separately
```bash
# Terminal 1
python run_server.py

# Terminal 2
python run_meters.py --meters 10 --interval 5
```

## Configuration (30 seconds)

Edit `config/system_config.yaml` to change:

```yaml
smart_meters:
  num_meters: 10              # How many meters
  reading_interval_sec: 5     # How often (seconds)
  batch_size: 5               # Readings per batch

ckks:
  poly_modulus_degree: 8192   # Security vs Speed (4096/8192/16384)
  coeff_modulus: 40           # Security (30-60)
```

## Command Reference

| Command | What It Does |
|---------|------------|
| `python main.py --mode demo` | 30-second demo |
| `python main.py --mode run --duration 60` | Run for 60 seconds |
| `python run_server.py` | Start server only |
| `python run_meters.py --meters 5` | Start 5 meters |
| `python run_benchmark.py` | Full benchmarks |
| `python run_benchmark.py --quick` | Quick test |
| `python validate_system.py` | Check setup |

## Understanding Output

### Meter Output
```
[Meter 0] Sent encrypted batch (5 readings). Encryption: 125.34ms, Communication: 8.23ms
```
- **125.34ms**: Time to encrypt 5 readings
- **8.23ms**: Network round-trip time

### Server Output
```
[Server] Received encrypted batch from meter 0 (5 readings, encrypted in 125.34ms)
```
- Server received 5 encrypted readings
- Meter took 125.34ms to encrypt

### Analytics Output
```
[Server] Computed encrypted sum of 50 readings from 10 meters in 45.67ms
```
- Computed homomorphic sum of 50 readings
- Operation took 45.67ms (without decryption!)

## Results Files

After running, find results in:
- `results/analytics_results.json` - HE computation results
- `results/performance_metrics.json` - Performance data
- `results/server_stats.json` - Server statistics
- `logs/system.log` - All events

## Performance Expectations

```
Encryption time:    50-500 ms per batch
Network latency:    5-20 ms
HE operations:      10-50 ms
Throughput:         100-1000 readings/sec
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "No module named 'tenseal'" | `pip install tenseal` |
| "Address already in use" | Change port: `--port 5001` |
| "Cannot connect" | Start server first |
| "Out of memory" | Reduce `num_meters` in config |
| "Encryption slow" | Reduce `poly_modulus_degree` |

## Testing Scenarios

### Light Test (Quick)
```yaml
smart_meters:
  num_meters: 2
  reading_interval_sec: 10
  batch_size: 1
```

### Normal Test
```yaml
smart_meters:
  num_meters: 10
  reading_interval_sec: 5
  batch_size: 5
```

### Heavy Load Test
```yaml
smart_meters:
  num_meters: 20
  reading_interval_sec: 1
  batch_size: 20
```

## Encryption Levels

### Fast (Testing)
```yaml
ckks:
  poly_modulus_degree: 4096
  coeff_modulus: 30
```

### Standard (Default)
```yaml
ckks:
  poly_modulus_degree: 8192
  coeff_modulus: 40
```

### Secure (Production)
```yaml
ckks:
  poly_modulus_degree: 16384
  coeff_modulus: 50
```

## Key Files to Know

| File | Purpose | Edit? |
|------|---------|-------|
| `config/system_config.yaml` | Configuration | YES |
| `common/utils.py` | Data structures | NO |
| `common/encryption.py` | CKKS engine | NO |
| `smart_meters/meter_client.py` | Meter logic | NO |
| `analytics_server/server.py` | Server logic | NO |
| `logger/performance_logger.py` | Benchmarking | NO |
| `main.py` | Orchestrator | NO |

## System Architecture at a Glance

```
Meter 1 ─┐
Meter 2 ─┼→ Encryption ─→ Network ─→ Server ─→ HE Analytics
Meter N ─┘                                    ─→ Logger
```

## What Happens Inside

```
1. Meter collects power reading (2150.3W)
2. Batch with 4 others: [2150.3, 2155.1, 2148.9, ...]
3. Encrypt with CKKS: CIPHERTEXT (3000+ bytes)
4. Send to server via TCP
5. Server stores encrypted data
6. Server computes HE sum WITHOUT decryption
7. Result stays encrypted
8. Logger tracks performance metrics
```

## Common Tasks

### Run 5 Minute Test
```bash
python main.py --mode run --duration 300
```

### Compare Encryption Methods
```bash
# Edit config: poly_modulus_degree: 4096
python main.py --mode demo
# Edit config: poly_modulus_degree: 8192
python main.py --mode demo
# Compare results in logs
```

### Test with Many Meters
```bash
# Edit config: num_meters: 50
python main.py --mode run --duration 60
# Results in ./results/
```

### Generate Performance Report
```bash
python run_benchmark.py
# Results in ./logs/benchmark_report.json
```

## Performance Tuning

### Slow Encryption?
```yaml
ckks:
  poly_modulus_degree: 4096    # Was 8192
```

### Want More Readings?
```yaml
smart_meters:
  reading_interval_sec: 1      # Was 5
  num_meters: 20               # Was 10
```

### Need Lower Latency?
```yaml
smart_meters:
  batch_size: 1                # Was 5 (sends more often)
```

## Files Modified After Installation

You will create:
- `results/` - Test results (auto-created)
- `logs/` - Log files (auto-created)
- Your custom config files (optional)

## Validation Commands

```bash
# Quick check
python validate_system.py

# Test meter connection
python run_meters.py --meters 2 --duration 5

# Test server
python run_server.py --duration 5

# Full smoke test
python main.py --mode demo
```

## Getting Help

1. **Setup Issues**: `python validate_system.py`
2. **How to Use**: Read `USAGE_GUIDE.md`
3. **How It Works**: Read `ARCHITECTURE.md`
4. **How It's Built**: Read `IMPLEMENTATION_GUIDE.md`
5. **Details**: Check docstrings in code

## One-Liner Examples

```bash
# Demo
python main.py --mode demo

# 2-minute test
python main.py --mode run --duration 120

# Quick benchmark
python run_benchmark.py --quick

# Full benchmark
python run_benchmark.py

# Just server
python run_server.py &

# Just meters (5 meters, 5 second interval)
python run_meters.py --meters 5 --interval 5

# Validate setup
python validate_system.py
```

## Expected Output Pattern

```
[System] Starting Smart Grid System...
[Server] Started on 127.0.0.1:5000
[Grid] Started 10 meters
[Meter 0] Sent encrypted batch (5 readings). Encryption: 125.34ms, Communication: 8.23ms
[Server] Received encrypted batch from meter 0 (5 readings, encrypted in 125.34ms)
[Server] Computed encrypted sum of 50 readings from 10 meters in 45.67ms
[System] System stopped
=== Final Statistics ===
Total readings: 500
Avg encryption time: 125.34ms
Throughput: 8.33 readings/sec
```

## Next Steps After Installation

1. Run: `python main.py --mode demo`
2. Read: `USAGE_GUIDE.md`
3. Experiment: Modify `config/system_config.yaml`
4. Benchmark: `python run_benchmark.py`
5. Analyze: Check `results/` and `logs/`

---

**TL;DR**: Install → Run `python main.py --mode demo` → Check results in `./results/`
