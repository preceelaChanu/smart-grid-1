#!/bin/bash

# Smart Grid Continuous Operation Test Script
# This script demonstrates the continuous smart grid system with periodic data transmission

echo "====================================================="
echo "          SMART GRID CONTINUOUS OPERATION"
echo "====================================================="

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Error: Build directory not found. Please run cmake and make first."
    exit 1
fi

cd build

# Check if all required binaries exist
REQUIRED_BINARIES=("keygen" "certgen" "kdc" "client_continuous" "aggregator_continuous" "control_center_continuous")
for binary in "${REQUIRED_BINARIES[@]}"; do
    if [ ! -f "$binary" ]; then
        echo "Error: $binary not found. Please build the project first."
        exit 1
    fi
done

echo "✓ All required binaries found"

# Clean previous data
echo "Cleaning previous test data..."
rm -rf keys/* data/* certificates/* *.log 2>/dev/null
mkdir -p keys data certificates

# Step 1: Generate encryption keys
echo ""
echo "1. Generating SEAL encryption keys..."
./keygen
if [ $? -ne 0 ]; then
    echo "Error: Key generation failed"
    exit 1
fi
echo "✓ Encryption keys generated"

# Step 2: Generate certificates
echo ""
echo "2. Generating network certificates..."
./certgen
if [ $? -ne 0 ]; then
    echo "Error: Certificate generation failed"
    exit 1
fi
echo "✓ Network certificates generated"

# Function to cleanup background processes
cleanup() {
    echo ""
    echo "========================================="
    echo "Shutting down smart grid system..."
    echo "========================================="
    
    # Send SIGTERM to all background processes
    for pid in "${PIDS[@]}"; do
        if ps -p $pid > /dev/null 2>&1; then
            echo "Stopping process $pid..."
            kill -TERM $pid 2>/dev/null
        fi
    done
    
    # Wait a moment for graceful shutdown
    sleep 3
    
    # Force kill any remaining processes
    for pid in "${PIDS[@]}"; do
        if ps -p $pid > /dev/null 2>&1; then
            echo "Force stopping process $pid..."
            kill -KILL $pid 2>/dev/null
        fi
    done
    
    echo "System shutdown complete"
    exit 0
}

# Setup signal handlers
trap cleanup SIGINT SIGTERM

# Array to store background process PIDs
PIDS=()

# Step 3: Start Key Distribution Center
echo ""
echo "3. Starting Key Distribution Center (KDC)..."
./kdc > kdc.log 2>&1 &
KDC_PID=$!
PIDS+=($KDC_PID)

# Wait for KDC to start
sleep 2

if ! ps -p $KDC_PID > /dev/null; then
    echo "Error: KDC failed to start"
    exit 1
fi
echo "✓ KDC started (PID: $KDC_PID)"

# Step 4: Start Control Center
echo ""
echo "4. Starting Continuous Control Center..."
./control_center_continuous > control_center.log 2>&1 &
CC_PID=$!
PIDS+=($CC_PID)

# Wait for control center to start
sleep 2

if ! ps -p $CC_PID > /dev/null; then
    echo "Error: Control Center failed to start"
    exit 1
fi
echo "✓ Control Center started (PID: $CC_PID)"

# Step 5: Start Aggregator
echo ""
echo "5. Starting Continuous Aggregator..."
./aggregator_continuous > aggregator.log 2>&1 &
AGGREGATOR_PID=$!
PIDS+=($AGGREGATOR_PID)

# Wait for aggregator to start
sleep 2

if ! ps -p $AGGREGATOR_PID > /dev/null; then
    echo "Error: Aggregator failed to start"
    exit 1
fi
echo "✓ Aggregator started (PID: $AGGREGATOR_PID)"

# Step 6: Start Smart Meters (as continuous clients)
echo ""
echo "6. Starting Continuous Smart Meters..."

# Read number of clients from config
NUM_CLIENTS=$(jq -r '.num_clients' ../config.json 2>/dev/null || echo "10")
echo "Starting $NUM_CLIENTS continuous smart meters..."

# Start smart meters with staggered startup
for i in $(seq 1 $NUM_CLIENTS); do
    echo "  Starting smart meter $i..."
    
    # Create instance-specific config
    cat > client_$i.config <<EOF
{
    "meter_id": $i,
    "base_port": 9000,
    "aggregator": {
        "host": "127.0.0.1",
        "port": 8080
    }
}
EOF
    
    # Start the smart meter
    ./client_continuous $i > client_$i.log 2>&1 &
    CLIENT_PID=$!
    PIDS+=($CLIENT_PID)
    
    # Brief delay to avoid overwhelming the system
    sleep 0.2
    
    # Check if client started successfully
    sleep 1
    if ! ps -p $CLIENT_PID > /dev/null; then
        echo "Warning: Smart meter $i may have failed to start"
    else
        echo "  ✓ Smart meter $i started (PID: $CLIENT_PID)"
    fi
done

echo ""
echo "====================================================="
echo "          CONTINUOUS OPERATION STARTED"
echo "====================================================="
echo "System Status:"
echo "  KDC Server: Running (PID: $KDC_PID)"
echo "  Control Center: Running (PID: $CC_PID)"
echo "  Aggregator: Running (PID: $AGGREGATOR_PID)"
echo "  Smart Meters: $NUM_CLIENTS running"
echo ""
echo "System Behavior (15x Accelerated for Testing):"
echo "  - Smart meters generate data every 20 seconds (simulates 5 minutes)"
echo "  - Aggregator collects data every 1 minute (simulates 15 minutes)"
echo "  - Control center performs analytics every 4 minutes (simulates 1 hour)"
echo "  - Energy consumption varies by time of day"
echo ""
echo "Log Files:"
echo "  - KDC: kdc.log"
echo "  - Control Center: control_center.log"
echo "  - Aggregator: aggregator.log"
echo "  - Smart Meters: client_N.log"
echo ""
echo "Monitoring Commands:"
echo "  tail -f control_center.log    # Control center activity"
echo "  tail -f aggregator.log        # Aggregation activity"
echo "  tail -f client_1.log          # Smart meter example"
echo ""
echo "Press Ctrl+C to stop the entire system gracefully"
echo "====================================================="

# Main monitoring loop
echo ""
echo "System is now running continuously..."
echo "Monitor the logs to see periodic data transmission and analytics..."

MONITORING_INTERVAL=30
MONITORING_COUNT=0

while true; do
    sleep $MONITORING_INTERVAL
    MONITORING_COUNT=$((MONITORING_COUNT + 1))
    
    # Check if all processes are still running
    RUNNING_COUNT=0
    for pid in "${PIDS[@]}"; do
        if ps -p $pid > /dev/null 2>&1; then
            RUNNING_COUNT=$((RUNNING_COUNT + 1))
        fi
    done
    
    echo ""
    echo "=== System Health Check #$MONITORING_COUNT ==="
    echo "Processes running: $RUNNING_COUNT / ${#PIDS[@]}"
    echo "Uptime: $(($MONITORING_COUNT * $MONITORING_INTERVAL)) seconds"
    
    # Show recent activity from control center log
    if [ -f control_center.log ]; then
        echo "Recent Control Center Activity:"
        tail -n 3 control_center.log | sed 's/^/  /'
    fi
    
    # Show recent activity from aggregator log
    if [ -f aggregator.log ]; then
        echo "Recent Aggregator Activity:"
        tail -n 3 aggregator.log | sed 's/^/  /'
    fi
    
    echo "=== End Health Check ==="
    
    # If too many processes have died, exit
    if [ $RUNNING_COUNT -lt $((${#PIDS[@]} / 2)) ]; then
        echo "Error: More than half of processes have stopped. Exiting..."
        cleanup
    fi
done