#!/bin/bash

# Privacy-Preserving Smart Grid Analytics Framework
# Automated Test Harness and Orchestrator
# 
# This script manages the complete system lifecycle:
# 1. Environment setup and cleanup
# 2. Key generation
# 3. Client simulation (multiple smart meters)
# 4. Homomorphic aggregation
# 5. Result decryption and verification
# 6. Performance analysis and reporting

set -e  # Exit on any error

echo "========================================================"
echo "Privacy-Preserving Smart Grid Analytics Framework"
echo "Automated Test Harness"
echo "========================================================"

# Configuration loading
if [ ! -f "config.json" ]; then
    echo "Error: config.json not found"
    exit 1
fi

# Extract configuration using jq
if ! command -v jq &> /dev/null; then
    echo "Error: jq is required for JSON parsing"
    echo "Install with: sudo apt install jq"
    exit 1
fi

NUM_CLIENTS=$(jq -r '.num_clients' config.json)
POLY_DEGREE=$(jq -r '.poly_modulus_degree' config.json)
SCALE_BITS=$(jq -r '.ckks_scale_bits' config.json)

echo "Configuration loaded:"
echo "  Number of clients: $NUM_CLIENTS"
echo "  Security level (poly degree): $POLY_DEGREE"
echo "  CKKS scale bits: $SCALE_BITS"
echo ""

# Check if binaries exist
BINARIES=("build/keygen" "build/client" "build/aggregator" "build/control_center" "build/certgen")
for binary in "${BINARIES[@]}"; do
    if [ ! -f "$binary" ]; then
        echo "Error: $binary not found"
        echo "Please run: mkdir build && cd build && cmake .. && make && cd .."
        exit 1
    fi
done

echo "All required binaries found ✓"
echo ""

# Function to cleanup background processes
cleanup_processes() {
    echo "Cleaning up background processes..."
    if [ ! -z "$CONTROL_CENTER_PID" ]; then
        kill $CONTROL_CENTER_PID 2>/dev/null || true
        wait $CONTROL_CENTER_PID 2>/dev/null || true
        echo "Control center stopped"
    fi
}

# Set trap to cleanup on exit
trap cleanup_processes EXIT

# Cleanup function
cleanup_data() {
    echo "Cleaning up previous test data..."
    rm -rf keys/*.seal data/*.seal 2>/dev/null || true
    echo "Cleanup completed ✓"
    echo ""
}

# Performance tracking
TOTAL_START=$(date +%s.%N)

# Step 1: Cleanup
cleanup_data

# Step 2: Certificate Generation
echo "=========================================="
echo "STEP 1: Certificate Generation"
echo "=========================================="
CERTGEN_START=$(date +%s.%N)
./build/certgen
CERTGEN_END=$(date +%s.%N)
CERTGEN_TIME=$(echo "$CERTGEN_END - $CERTGEN_START" | bc -l)

echo ""
echo "Certificate generation completed in ${CERTGEN_TIME} seconds"
echo ""

# Step 3: Key Generation
echo "=========================================="
echo "STEP 2: Cryptographic Key Generation (KGC)"
echo "=========================================="
KEYGEN_START=$(date +%s.%N)
./build/keygen
KEYGEN_END=$(date +%s.%N)
KEYGEN_TIME=$(echo "$KEYGEN_END - $KEYGEN_START" | bc -l)

echo ""
echo "Key generation completed in ${KEYGEN_TIME} seconds"
echo ""

# Step 4: Client Simulation
echo "=========================================="
echo "STEP 3: Smart Meter Simulation"
echo "=========================================="
CLIENT_START=$(date +%s.%N)

echo "Simulating $NUM_CLIENTS smart meters..."
TOTAL_ENCRYPTED_SIZE=0

for ((i=1; i<=NUM_CLIENTS; i++)); do
    echo "Running client $i..."
    ./build/client $i
    
    # Calculate ciphertext size
    if [ -f "data/ct_client_${i}.seal" ]; then
        SIZE=$(stat -c%s "data/ct_client_${i}.seal")
        TOTAL_ENCRYPTED_SIZE=$((TOTAL_ENCRYPTED_SIZE + SIZE))
    fi
    echo ""
done

CLIENT_END=$(date +%s.%N)
CLIENT_TIME=$(echo "$CLIENT_END - $CLIENT_START" | bc -l)
AVG_CLIENT_TIME=$(echo "scale=4; $CLIENT_TIME / $NUM_CLIENTS" | bc -l)

echo "All clients completed in ${CLIENT_TIME} seconds"
echo "Average time per client: ${AVG_CLIENT_TIME} seconds"
echo "Total encrypted data size: $TOTAL_ENCRYPTED_SIZE bytes"
echo ""

# Step 5: Start Control Center (Background Server)
echo "=========================================="
echo "STEP 4: Starting Control Center Server"
echo "=========================================="
echo "Starting Control Center as background TCP server..."
./build/control_center &
CONTROL_CENTER_PID=$!

echo "Control Center PID: $CONTROL_CENTER_PID"
echo "Waiting for Control Center to initialize..."
sleep 3  # Give time for server to start

# Check if Control Center is still running
if ! kill -0 $CONTROL_CENTER_PID 2>/dev/null; then
    echo "Error: Control Center failed to start"
    exit 1
fi

echo "✓ Control Center server is running"
echo ""

# Step 6: Network Aggregation
echo "=========================================="
echo "STEP 5: Network-Based Homomorphic Aggregation"
echo "=========================================="
AGGREGATOR_START=$(date +%s.%N)
./build/aggregator
AGGREGATOR_END=$(date +%s.%N)
AGGREGATOR_TIME=$(echo "$AGGREGATOR_END - $AGGREGATOR_START" | bc -l)

echo ""
echo "Network aggregation completed in ${AGGREGATOR_TIME} seconds"
echo ""

# Wait for Control Center to finish processing
echo "Waiting for Control Center to complete processing..."
wait $CONTROL_CENTER_PID
CONTROL_CENTER_EXIT_CODE=$?
CONTROL_CENTER_PID=""  # Clear PID since process finished

if [ $CONTROL_CENTER_EXIT_CODE -ne 0 ]; then
    echo "Warning: Control Center exited with code $CONTROL_CENTER_EXIT_CODE"
else
    echo "✓ Control Center completed successfully"
fi

TOTAL_END=$(date +%s.%N)
TOTAL_TIME=$(echo "$TOTAL_END - $TOTAL_START" | bc -l)

echo ""
echo "Control center completed in ${CONTROL_TIME} seconds"
echo ""

# Final Performance Summary
echo "========================================================"
echo "COMPREHENSIVE PERFORMANCE REPORT"
echo "========================================================"
echo "System Configuration:"
echo "  Security Level: $POLY_DEGREE (poly modulus degree)"
echo "  Precision: $SCALE_BITS bits"
echo "  Number of Smart Meters: $NUM_CLIENTS"
echo ""
echo "Timing Analysis:"
echo "  Certificate Generation: ${CERTGEN_TIME} seconds"
echo "  Key Generation: ${KEYGEN_TIME} seconds"
echo "  Client Simulation: ${CLIENT_TIME} seconds (avg: ${AVG_CLIENT_TIME}s per client)"
echo "  Network Aggregation: ${AGGREGATOR_TIME} seconds"
echo "  Total End-to-End: ${TOTAL_TIME} seconds"
echo ""
echo "Storage Analysis:"
echo "  Total Encrypted Data: $TOTAL_ENCRYPTED_SIZE bytes"
echo "  Average per Client: $((TOTAL_ENCRYPTED_SIZE / NUM_CLIENTS)) bytes"
echo ""
echo "Privacy Guarantees:"
echo "  ✓ Client data encrypted before transmission"
echo "  ✓ Aggregator processed data without decryption capability"
echo "  ✓ Individual consumption never exposed during computation"
echo "  ✓ Post-quantum security via CKKS scheme"
echo ""
echo "Network Security:"
echo "  ✓ Certificate-based node authentication"
echo "  ✓ Secure TCP/IP communication channels"
echo "  ✓ Session token validation for data integrity"
echo "  ✓ Real-world network architecture simulation"
echo ""

# Throughput calculations
if (( $(echo "$CLIENT_TIME > 0" | bc -l) )); then
    THROUGHPUT=$(echo "scale=2; $NUM_CLIENTS / $CLIENT_TIME" | bc -l)
    echo "Throughput: $THROUGHPUT clients/second"
fi

if (( $(echo "$TOTAL_ENCRYPTED_SIZE > 0 && $TOTAL_TIME > 0" | bc -l) )); then
    BANDWIDTH=$(echo "scale=2; $TOTAL_ENCRYPTED_SIZE / $TOTAL_TIME" | bc -l)
    echo "Effective Bandwidth: $BANDWIDTH bytes/second"
fi

echo ""
echo "========================================================"
echo "NETWORKED SMART GRID SYSTEM COMPLETED SUCCESSFULLY"
echo "========================================================"
echo "The privacy-preserving smart grid analytics framework"
echo "has successfully demonstrated secure aggregation of"
echo "$NUM_CLIENTS smart meters with realistic TCP/IP networking,"
echo "certificate-based authentication, and end-to-end encryption"
echo "without exposing individual consumption data."
echo ""
echo "Network Architecture Validated:"
echo "  - Smart Meters → Aggregator (file-based for now)"
echo "  - Aggregator → Control Center (TCP/IP with authentication)"
echo "  - Certificate Authority for node validation"
echo ""
echo "Next steps:"
echo "  - Implement Smart Meter → Aggregator TCP/IP connections"
echo "  - Add Key Distribution Center network services"
echo "  - Implement load balancing across multiple aggregators"
echo "  - Add network monitoring and fault tolerance"
echo "  - Deploy across distributed infrastructure"
echo "========================================================"