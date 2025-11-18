# Quick Start Guide

## Overview
This Privacy-Preserving Smart Grid Analytics Framework implements a complete CKKS-based homomorphic encryption system for secure smart grid data aggregation.

## Architecture Summary

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐    ┌──────────────────┐
│ Key Generation  │───▶│   Smart Meters   │───▶│   Aggregator    │───▶│ Control Center   │
│    Center       │    │   (Clients)      │    │    Node         │    │                  │
│                 │    │                  │    │                  │    │                  │
│ • Generate PK   │    │ • Encrypt data   │    │ • Sum encrypted │    │ • Decrypt result │
│ • Generate SK   │    │ • Use only PK    │    │ • Use PK + RLK  │    │ • Use only SK    │
│ • Generate RLK  │    │ • No SK access   │    │ • No SK access  │    │ • Final analytics│
└─────────────────┘    └──────────────────┘    └─────────────────┘    └──────────────────┘
```

## Quick Start (3 Steps)

### Step 1: Install Dependencies
```bash
# For Ubuntu/Debian systems
./install_dependencies.sh
```

### Step 2: Build the System
```bash
mkdir build && cd build
cmake ..
make
cd ..
```

### Step 3: Run the Framework
```bash
./run_test.sh
```

That's it! The system will automatically:
1. Generate cryptographic keys
2. Simulate 10 smart meters
3. Perform privacy-preserving aggregation
4. Decrypt and display results with performance metrics

## Expected Output

```
========================================
STEP 1: Key Generation (KGC)
========================================
=== Key Generation Center (KGC) ===
Loading configuration...
Poly modulus degree: 8192
CKKS scale bits: 40
...

========================================
STEP 2: Smart Meter Simulation
========================================
=== Smart Meter Client 1 ===
Generated energy consumption: 2.345 kWh
Encryption time: 1250 μs
...

========================================
STEP 3: Homomorphic Aggregation
========================================
=== Aggregator Node (Blind Processor) ===
Successfully loaded 10 ciphertexts
Homomorphic aggregation time: 15 ms
PRIVACY PRESERVED: Aggregator never saw plaintext data!
...

========================================
STEP 4: Result Decryption & Analytics
========================================
=== Smart Grid Analytics Results ===
Total Energy Consumption: 25.647 kWh
Average per Smart Meter: 2.565 kWh
✓ Result is within expected range - VERIFICATION PASSED
```

## Configuration Options

Edit `config.json` to customize:

```json
{
  "poly_modulus_degree": 8192,    // Security: 8192=fast, 16384=strong, 32768=paranoid
  "ckks_scale_bits": 40,          // Precision: 30=fast, 40=balanced, 50=precise
  "num_clients": 10               // Scale: 10=demo, 100=test, 1000=production
}
```

## Manual Operation

If you prefer running components individually:

```bash
# 1. Generate keys
./build/keygen

# 2. Run clients (replace 1 with client ID)
./build/client 1
./build/client 2
# ... repeat for all clients

# 3. Aggregate
./build/aggregator

# 4. Decrypt results
./build/control_center
```

## File Structure

```
smart_grid_benchmark/
├── config.json          # Central configuration
├── CMakeLists.txt        # Build configuration
├── run_test.sh          # Automated orchestrator
├── keygen/keygen.cpp    # Key Generation Center
├── client/client.cpp    # Smart Meter simulator
├── aggregator/aggregator.cpp # Privacy-preserving aggregator
├── control_center/control_center.cpp # Final decryption
├── keys/                # Generated cryptographic keys
└── data/                # Encrypted smart meter data
```

## Security Guarantees

✅ **Client Privacy**: Individual energy consumption never exposed  
✅ **Computation Privacy**: Aggregator processes encrypted data without decryption capability  
✅ **Post-Quantum Security**: CKKS scheme resistant to quantum attacks  
✅ **Key Separation**: Different nodes have access to different keys only  

## Performance Expectations

| Clients | Key Gen | Encryption | Aggregation | Decryption | Total |
|---------|---------|------------|-------------|------------|-------|
| 10      | ~100ms  | ~10ms     | ~10ms       | ~5ms       | ~1s   |
| 100     | ~100ms  | ~100ms    | ~50ms       | ~5ms       | ~5s   |
| 1000    | ~100ms  | ~1s       | ~500ms      | ~5ms       | ~30s  |

*Times are approximate and depend on hardware*

## Next Steps

1. **Scale Testing**: Increase `num_clients` in config.json
2. **Real Data**: Integrate with actual smart meter CSV data  
3. **Advanced Analytics**: Implement billing calculations, anomaly detection
4. **Production Deployment**: Set up distributed architecture across networks
5. **Performance Tuning**: Optimize parameters for your specific requirements

## Troubleshooting

**Build Errors**: Ensure Microsoft SEAL is installed (`./install_dependencies.sh`)  
**Runtime Errors**: Check that all binaries exist in `build/` directory  
**Key Errors**: Run `./build/keygen` before other components  
**Permission Errors**: Ensure scripts are executable (`chmod +x *.sh`)

## Support

For detailed configuration options, see `ADVANCED_CONFIGURATION.md`  
For implementation details, see the comprehensive `README.md`

---

**🔒 Privacy-Preserving Smart Grid Analytics Framework**  
*Secure • Scalable • Post-Quantum Ready*