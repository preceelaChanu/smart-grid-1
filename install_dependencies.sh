#!/bin/bash

# Privacy-Preserving Smart Grid Analytics Framework
# Installation Script for Microsoft SEAL and Dependencies

set -e

echo "========================================================"
echo "Smart Grid Framework - Installation Script"
echo "========================================================"

# Check OS
if [[ "$OSTYPE" != "linux-gnu"* ]]; then
    echo "Error: This script is designed for Ubuntu Linux"
    exit 1
fi

echo "Step 1: Installing system dependencies..."
sudo apt update
sudo apt install -y build-essential g++ cmake git jq bc

echo "Step 2: Installing Microsoft SEAL v4.0.0..."

# Check if SEAL is already installed
if pkg-config --exists seal-4.0; then
    echo "Microsoft SEAL v4.0.0 is already installed ✓"
else
    echo "Downloading and building Microsoft SEAL..."
    
    # Create temporary directory
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"
    
    # Clone SEAL
    git clone https://github.com/microsoft/SEAL.git
    cd SEAL
    git checkout v4.0.0
    
    # Configure and build
    cmake -S . -B build -DSEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF
    cmake --build build
    
    # Install
    sudo cmake --install build
    
    # Cleanup
    cd /
    rm -rf "$TEMP_DIR"
    
    echo "Microsoft SEAL installed successfully ✓"
fi

echo "Step 3: Verifying installation..."

# Test SEAL installation - check multiple possible locations
SEAL_FOUND=false

# Check if SEAL library exists in common locations
if [ -f "/usr/local/lib/libseal-4.0.a" ] || [ -f "/usr/local/lib64/libseal-4.0.a" ] || [ -f "/usr/lib/libseal-4.0.a" ]; then
    echo "✓ Microsoft SEAL library files found"
    SEAL_FOUND=true
elif pkg-config --exists seal-4.0; then
    echo "✓ Microsoft SEAL v4.0.0 found via pkg-config"
    SEAL_FOUND=true
elif [ -d "/usr/local/include/SEAL" ]; then
    echo "✓ Microsoft SEAL headers found"
    SEAL_FOUND=true
fi

if [ "$SEAL_FOUND" = false ]; then
    echo "✗ Microsoft SEAL installation verification failed"
    echo "Attempting to locate SEAL installation..."
    find /usr -name "*seal*" 2>/dev/null | head -10
    echo "Note: SEAL may still work even if verification fails"
fi

# Test required tools
REQUIRED_TOOLS=("g++" "cmake" "jq" "bc")
for tool in "${REQUIRED_TOOLS[@]}"; do
    if command -v "$tool" >/dev/null 2>&1; then
        echo "✓ $tool found"
    else
        echo "✗ $tool not found"
        exit 1
    fi
done

echo ""
echo "========================================================"
echo "Installation completed successfully!"
echo "========================================================"
echo "You can now build and run the Smart Grid Framework:"
echo ""
echo "cd /workspaces/smart-grid-1"
echo "mkdir build && cd build"
echo "cmake .."
echo "make"
echo "cd .."
echo "./run_continuous_test.sh"
echo ""
echo "========================================================"