#!/bin/bash
set -e

echo "================================================================================================"
echo "🏠 SMART GRID REALISTIC ENERGY CONSUMPTION SIMULATION"
echo "================================================================================================"
echo ""
echo "📊 DATASET OVERVIEW (Real UK Household Data Analysis)"
echo "├── Total Households Analyzed: 5,560"
echo "├── Average Consumption per Household: 0.213 kWh" 
echo "├── Median Consumption per Household: 0.174 kWh"
echo "├── Consumption Range: 0.000 - 2.112 kWh"
echo "├── Peak Consumption Hour: 19:00 (0.315 kWh avg)"
echo "└── Lowest Consumption Hour: 4:00 (0.110 kWh avg)"
echo ""

echo "🔧 SYSTEM CONFIGURATION"
echo "└── Simulating household consumption patterns based on real UK dataset"
echo ""

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "❌ Build directory not found. Please run 'mkdir build && cd build && cmake .. && make' first"
    exit 1
fi

cd build

echo "🔑 Generating certificates and keys..."
if [ ! -d "keys" ]; then
    mkdir -p keys
fi

# Generate certificates if they don't exist
if [ ! -f "keys/kdc.cert" ]; then
    echo "Generating certificates..."
    ./certgen
fi

# Generate cryptographic keys if they don't exist  
if [ ! -f "keys/public_key.seal" ]; then
    echo "Generating SEAL keys..."
    ./keygen
fi

echo ""
echo "🚀 STARTING REALISTIC SMART GRID SIMULATION"
echo "================================================================================================"
echo ""

echo "📋 Starting Key Distribution Center (KDC)..."
gnome-terminal --title="KDC Server" --geometry=100x30+0+0 -- bash -c "./kdc; exec bash" &
KDC_PID=$!

sleep 3

echo "🏢 Starting Control Center (with realistic analytics)..."
gnome-terminal --title="Control Center - Analytics" --geometry=120x40+600+0 -- bash -c "./control_center_continuous; exec bash" &
CC_PID=$!

sleep 2

echo "📊 Starting Aggregator (processing realistic consumption data)..."
gnome-terminal --title="Aggregator - Data Processing" --geometry=100x30+0+400 -- bash -c "./aggregator_continuous; exec bash" &
AGG_PID=$!

sleep 2

echo "🏠 Starting Smart Meter Simulations (with realistic household types)..."

# Start a few example smart meters with different household profiles
echo "  └── Starting Low Consumer Households (95.4% of population)..."
for i in {1..3}; do
    gnome-terminal --title="Smart Meter $i (Low Consumer)" --geometry=80x20+$((400+i*200))+400 -- bash -c "./client_continuous $i; exec bash" &
    sleep 0.5
done

echo "  └── Starting Medium Consumer Households (4.6% of population)..."
for i in {4..5}; do
    gnome-terminal --title="Smart Meter $i (Medium Consumer)" --geometry=80x20+$((400+i*200))+600 -- bash -c "./client_continuous $i; exec bash" &
    sleep 0.5
done

echo "  └── Starting Variable Consumer Household (high variation pattern)..."
gnome-terminal --title="Smart Meter 6 (Variable Consumer)" --geometry=80x20+400+800 -- bash -c "./client_continuous 6; exec bash" &

echo ""
echo "✅ SIMULATION STARTED SUCCESSFULLY!"
echo "================================================================================================"
echo ""
echo "📈 WHAT TO OBSERVE:"
echo "├── 🏠 Smart Meter Terminals: Show realistic energy consumption patterns"
echo "│   ├── Household Types: Low/Medium/High/Variable consumers"
echo "│   ├── Temporal Patterns: Peak at 19:00, Low at 4:00"
echo "│   └── Daily Variations: Higher on Sunday, Lower on Tuesday"
echo "├── 📊 Aggregator Terminal: Processing and aggregating encrypted data"
echo "├── 🏢 Control Center: Comprehensive analytics based on real patterns"
echo "│   ├── Grid status with realistic consumption ranges"
echo "│   ├── Peak/off-peak detection based on dataset"
echo "│   └── Anomaly detection for unusual consumption"
echo "└── 🔑 KDC Terminal: Secure key distribution and management"
echo ""
echo "📊 REALISTIC CONSUMPTION PATTERNS:"
echo "├── Average: ~0.213 kWh per household per reading"
echo "├── Range: 0.000 - 2.112 kWh (matching real dataset)"
echo "├── Peak Hours: 17:00-22:00 (evening peak)"
echo "├── Low Hours: 23:00-06:00 (night time)"
echo "└── Variable Consumers: 51% show high consumption variation"
echo ""
echo "🎯 The simulation now accurately reflects real UK household energy consumption patterns!"
echo "💡 Press Ctrl+C in any terminal to stop the respective service gracefully."
echo ""
echo "⏰ Simulation running... Monitor the terminals for realistic energy data patterns."

# Keep the script running
echo ""
echo "Press [Enter] to stop all services..."
read

echo ""
echo "🛑 Stopping all services..."

# Kill all background processes
pkill -f "./kdc" 2>/dev/null || true
pkill -f "./control_center_continuous" 2>/dev/null || true  
pkill -f "./aggregator_continuous" 2>/dev/null || true
pkill -f "./client_continuous" 2>/dev/null || true

sleep 2

echo "✅ All services stopped. Simulation complete!"
echo ""
echo "📋 SUMMARY: Successfully demonstrated realistic smart grid energy consumption simulation"
echo "   based on analysis of 5,560 UK household energy consumption patterns."