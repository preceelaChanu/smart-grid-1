# Advanced Configuration Guide

## Security Parameter Tuning

### Polynomial Modulus Degree
Controls the security level and performance:

```json
{
  "poly_modulus_degree": 8192   // Fast (128-bit security)
  "poly_modulus_degree": 16384  // Strong (192-bit security) 
  "poly_modulus_degree": 32768  // Paranoid (256-bit security)
}
```

**Trade-offs:**
- Higher values = Better security + Larger ciphertexts + Slower computation
- Lower values = Faster computation + Smaller ciphertexts + Lower security

### CKKS Scale Bits
Controls precision of real number arithmetic:

```json
{
  "ckks_scale_bits": 30   // Lower precision, faster
  "ckks_scale_bits": 40   // Balanced (recommended)
  "ckks_scale_bits": 50   // Higher precision, slower
}
```

## Performance Optimization

### Client Scaling
For large-scale deployments:

```json
{
  "num_clients": 1000     // Test with 1K smart meters
  "num_clients": 10000    // Production scale
}
```

### Parallel Processing
Modify `run_test.sh` for concurrent client simulation:

```bash
# Sequential (current)
for ((i=1; i<=NUM_CLIENTS; i++)); do
    ./build/client $i
done

# Parallel (faster)
for ((i=1; i<=NUM_CLIENTS; i++)); do
    ./build/client $i &
done
wait
```

## Real-World Data Integration

### CSV Data Format
Expected format for smart meter readings:

```csv
meter_id,timestamp,consumption_kwh
001,2025-11-18T10:00:00,2.345
002,2025-11-18T10:00:00,1.876
```

### Client Modification Example
Modify `client/client.cpp` to read from CSV:

```cpp
// Replace random data generation with:
ifstream csv_file("smart_meter_data.csv");
string line;
getline(csv_file, line); // Skip header
getline(csv_file, line);
// Parse CSV line to extract consumption value
```

## Network Deployment

### Distributed Architecture
For production deployment across different machines:

1. **Key Generation Center (KGC)**
   - Separate secure machine
   - Generate keys once
   - Distribute public keys via secure channels

2. **Client Nodes (Smart Meters)**
   - Embedded devices or edge computers
   - Only need public key
   - Encrypt and transmit data

3. **Aggregator Nodes**
   - Cloud or fog computing infrastructure
   - Multiple nodes for load balancing
   - No secret key access

4. **Control Center**
   - Secure data center
   - Only node with secret key
   - Final analytics and reporting

### Security Considerations

#### Key Management
- Store secret key in hardware security module (HSM)
- Use secure key distribution protocols
- Implement key rotation policies

#### Communication Security
- Use TLS/DTLS for data transmission
- Implement mutual authentication
- Add message integrity checks

#### Access Control
- Role-based access control (RBAC)
- Audit logging
- Secure multi-party computation for key generation

## Advanced Analytics

### Additional Homomorphic Operations

#### Weighted Averaging
```cpp
// In aggregator, multiply by weights before summing
vector<double> weights = {1.2, 0.8, 1.5, ...}; // Based on meter priority
for (size_t i = 0; i < client_ciphertexts.size(); i++) {
    Plaintext weight_plain;
    encoder.encode(weights[i], scale, weight_plain);
    evaluator.multiply_plain_inplace(client_ciphertexts[i], weight_plain);
    evaluator.relinearize_inplace(client_ciphertexts[i], relin_keys);
}
```

#### Statistical Moments
```cpp
// Compute variance homomorphically
Ciphertext sum_of_squares;
for (auto& ct : client_ciphertexts) {
    Ciphertext squared;
    evaluator.square(ct, squared);
    evaluator.relinearize_inplace(squared, relin_keys);
    if (sum_of_squares.size() == 0) {
        sum_of_squares = squared;
    } else {
        evaluator.add_inplace(sum_of_squares, squared);
    }
}
```

#### Threshold Detection
```cpp
// Detect meters above threshold without revealing individual values
double threshold = 4.0; // kWh
Plaintext threshold_plain;
encoder.encode(-threshold, scale, threshold_plain);

for (auto& ct : client_ciphertexts) {
    evaluator.add_plain_inplace(ct, threshold_plain);
    // Result positive if above threshold
}
```

## Benchmarking and Testing

### Performance Testing
Create `benchmark.sh` for systematic performance analysis:

```bash
#!/bin/bash
for degree in 8192 16384 32768; do
    for clients in 10 100 1000; do
        # Update config.json
        jq ".poly_modulus_degree = $degree | .num_clients = $clients" config.json > temp.json
        mv temp.json config.json
        
        # Run benchmark
        echo "Testing: degree=$degree, clients=$clients"
        ./run_test.sh > "results_${degree}_${clients}.log"
    done
done
```

### Accuracy Testing
Verify CKKS approximation errors:

```cpp
// In control_center.cpp, add precision analysis
double expected_total = 0;
for (int i = 1; i <= num_clients; i++) {
    expected_total += original_values[i]; // If known
}
double precision_error = abs(total_energy - expected_total);
double relative_error = precision_error / expected_total;
cout << "Absolute error: " << precision_error << " kWh" << endl;
cout << "Relative error: " << (relative_error * 100) << "%" << endl;
```

## Troubleshooting

### Common Issues

1. **SEAL Context Invalid**
   - Check polynomial modulus degree is power of 2
   - Verify coefficient modulus chain length
   - Ensure consistent parameters across all nodes

2. **Key Loading Errors**
   - Run keygen before clients/aggregator
   - Check file permissions and paths
   - Verify SEAL version compatibility

3. **Memory Issues with Large Polynomial Degrees**
   - Reduce polynomial modulus degree
   - Implement ciphertext batching
   - Use streaming for large datasets

4. **Performance Bottlenecks**
   - Profile with timing tools
   - Optimize coefficient modulus selection
   - Consider approximate computation strategies

### Debug Mode
Compile with debug flags:

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

### Logging Enhancement
Add detailed logging to all components:

```cpp
#ifdef DEBUG
    cout << "DEBUG: Ciphertext noise budget: " 
         << decryptor.invariant_noise_budget(encrypted) << " bits" << endl;
#endif
```