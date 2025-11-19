#!/bin/bash

# Scale Testing Script for Smart Grid Network
# Tests the system with varying numbers of smart meters

echo "========================================================"
echo "Smart Grid Scale Testing Suite"
echo "========================================================"

# Scale test configurations
SCALE_TESTS=(10 50 100 250 500)

for scale in "${SCALE_TESTS[@]}"; do
    echo ""
    echo "=========================================="
    echo "SCALE TEST: $scale Smart Meters"
    echo "=========================================="
    
    # Update configuration for this scale
    jq ".num_clients = $scale" config.json > config_temp.json
    mv config_temp.json config.json
    
    echo "Updated configuration for $scale smart meters"
    
    # Run the test
    START_TIME=$(date +%s.%N)
    ./run_test.sh > "scale_test_${scale}_meters.log" 2>&1
    END_TIME=$(date +%s.%N)
    
    # Calculate duration
    DURATION=$(echo "$END_TIME - $START_TIME" | bc -l)
    
    # Extract key metrics from log
    RUNNING_METERS=$(grep "smart meters are running" "scale_test_${scale}_meters.log" | grep -o '[0-9]\+/[0-9]\+' | head -1)
    
    echo "Scale test complete for $scale meters"
    echo "Duration: ${DURATION} seconds"
    echo "Result: $RUNNING_METERS meters deployed"
    echo "Log saved to: scale_test_${scale}_meters.log"
    
    # Cleanup between tests
    sleep 5
done

echo ""
echo "========================================================"
echo "SCALE TESTING COMPLETE"
echo "========================================================"
echo "Scale test results:"

for scale in "${SCALE_TESTS[@]}"; do
    if [ -f "scale_test_${scale}_meters.log" ]; then
        RUNNING_METERS=$(grep "smart meters are running" "scale_test_${scale}_meters.log" | grep -o '[0-9]\+/[0-9]\+' | head -1)
        echo "  $scale meters: $RUNNING_METERS deployed"
    fi
done

echo ""
echo "Individual test logs available:"
for scale in "${SCALE_TESTS[@]}"; do
    if [ -f "scale_test_${scale}_meters.log" ]; then
        echo "  - scale_test_${scale}_meters.log"
    fi
done
echo "========================================================"