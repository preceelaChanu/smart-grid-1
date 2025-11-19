#!/bin/bash

echo "=== Debug Test - Smart Grid Network ==="

# Make sure we have clean keys
echo "1. Generating keys..."
./build/keygen
echo "Keys generated ✓"

echo ""
echo "2. Generating certificates..."
./build/certgen
echo "Certificates generated ✓"

echo ""
echo "3. Starting single smart meter on port 9001..."
./build/client 1 &
METER_PID=$!
echo "Smart meter PID: $METER_PID"

echo ""
echo "4. Waiting for meter to initialize..."
sleep 5

echo ""
echo "5. Checking if meter is still running..."
if kill -0 $METER_PID 2>/dev/null; then
    echo "✓ Smart meter is running on PID $METER_PID"
else
    echo "✗ Smart meter failed or exited"
    exit 1
fi

echo ""
echo "6. Starting control center in background..."
./build/control_center &
CONTROL_PID=$!
echo "Control center PID: $CONTROL_PID"

sleep 3

echo ""
echo "7. Running aggregator to collect data..."
echo "About to run: ./build/aggregator"
./build/aggregator
AGGREGATOR_EXIT=$?

echo ""
echo "8. Aggregator finished with exit code: $AGGREGATOR_EXIT"

echo ""
echo "9. Cleaning up..."
kill $METER_PID $CONTROL_PID 2>/dev/null || true
wait $METER_PID $CONTROL_PID 2>/dev/null || true

echo ""
echo "=== Debug Test Complete ==="