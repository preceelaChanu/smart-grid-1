Privacy-Preserving Smart Grid Analytics Framework

# Overview

This project is a real-time framework designed to benchmark the performance and feasibility of Post-Quantum Homomorphic Encryption (HE) algorithms within a Smart Grid environment. It simulates a realistic, privacy-preserving 3-tier architecture where Smart Meters (Clients) encrypt their real-number energy consumption data (e.g., 1.234 kWh) before sending it to an Aggregator for processing, and finally to a Control Center for result decryption.

The system utilizes the CKKS (Cheon-Kim-Kim-Song) scheme from the Microsoft SEAL library. CKKS is a "levelled" HE scheme that supports approximate arithmetic on encrypted real numbers, making it ideal for smart grid analytics such as aggregation, weighted averages, and billing calculations without revealing individual user data.

# System Architecture

The project simulates a secure data flow model common in privacy-preserving smart grid literature (e.g., Zhou et al., Wagh et al.):

graph TD
    KGC[Key Generation Center] -- Generates Keys --> Keys[(Key Storage)]
    Keys -- Public Key --> Client[Smart Meter (Client)]
    Keys -- Public Key + Relin Keys --> Agg[Aggregator Node]
    Keys -- Secret Key --> Control[Control Center]
    
    Client -- 1. Encrypts Data --> Data[(Encrypted Data Files)]
    Data -- 2. Loads Ciphertexts --> Agg
    Agg -- 3. Homomorphic Aggregation --> AggResult[Aggregated Ciphertext]
    AggResult -- 4. Sends Result --> Control
    Control -- 5. Decrypts Result --> Final[Plaintext Total]


# Functional Architecture: Role of Each Node

The framework is managed by four distinct executable programs, ensuring a strict separation of concerns and cryptographic keys.

| Program | Logical Node Type | Role & Key Access | Primary Action(s) |
|---------|-------------------|-------------------|-------------------|
| keygen | KGC (Key Generation Center) | Trusted Authority. Generates all keys based on config.json and distributes them via disk. | Generates PK, SK, and RLK once at setup. |
| client | Smart Meter | Data Producer. Uses the Public Key (PK) to encrypt sensitive usage data. | 1. Encrypts real-number data (CKKS).<br>2. Saves unique ciphertext to a file. |
| aggregator | Middleware / Fog Node | Blind Processor. Uses PK and Relinearization Keys (RLK) for computation. Crucially, does not have the Secret Key. | 1. Loads all client ciphertexts.<br>2. Performs Homomorphic Aggregation.<br>3. Saves the single aggregated result. |
| control_center | Analytics Center | Final Manager / Decryptor. Only node with the Secret Key (SK). | 1. Loads the final aggregated ciphertext.<br>2. Decrypts it to reveal the total sum.<br>3. Decodes the result for verification. |

# Security Guarantee: 

The privacy of the system rests on the fact that the Aggregator, which performs the computations on all user data, does not possess the Secret Key. It operates entirely on encrypted data.

# Project Structure

smart_grid_benchmark/
├── CMakeLists.txt       # Master build configuration
├── config.json          # Central configuration for security & simulation settings
├── run_test.sh          # Automated test harness script (Orchestrator)
├── include/             # External headers (nlohmann/json)
├── keygen/
│   └── keygen.cpp       # Source code for Key Generation Center (CKKS)
├── client/
│   └── client.cpp       # Source code for Smart Meter Simulator (CKKS)
├── aggregator/
│   └── aggregator.cpp   # Source code for Aggregator Node (CKKS)
├── control_center/
│   └── control_center.cpp # Source code for Control Center (CKKS)
├── keys/                # Storage for generated keys (PK, SK, RLK)
└── data/                # Storage for encrypted smart meter data


# Prerequisites

Before running the framework, ensure your system has the following installed:

OS: Ubuntu Linux (20.04 or 22.04 recommended)

Compiler: g++ (supporting C++17)

Build System: cmake (version 3.16+)

Utilities: git, jq (for JSON parsing in scripts)

sudo apt update
sudo apt install build-essential g++ cmake git jq


🔧 Installation

1. Clone the Repository

mkdir smart_grid_benchmark
cd smart_grid_benchmark
## (If you are using git, clone here. Otherwise, create the folders manually)


2. Install Microsoft SEAL (v4.0.0)

The project relies on the Microsoft SEAL library.

## Clone and build SEAL inside the project or externally
git clone [https://github.com/microsoft/SEAL.git](https://github.com/microsoft/SEAL.git)
cd SEAL
git checkout v4.0.0
cmake -S . -B build -DSEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF
cmake --build build
sudo cmake --install build
cd ..


3. Install JSON Helper

The project uses nlohmann/json for configuration parsing.

mkdir -p include
wget -O include/json.hpp [https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp](https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp)


4. Build the Framework

mkdir build
cd build
cmake ..
make
cd ..


# Usage

You can run the components individually or use the automated test harness.

Option A: Automated Test Harness (Recommended)

The run_test.sh script acts as the Utility Orchestrator. It manages the entire lifecycle: cleaning old data, generating new keys, simulating N clients, running the aggregator, and verifying the result.

chmod +x run_test.sh
./run_test.sh


Option B: Manual Execution

Generate Keys:

./build/keygen


Run Clients: (Replace 1 with the Client ID)

./build/client 1
./build/client 2


Run Aggregator:

./build/aggregator


Run Control Center:

./build/control_center


# Configuration (config.json)

All experiment variables are controlled via config.json. You do not need to recompile the code to change these settings.

{
  "comment_security": "poly_modulus_degree: 8192 (fast), 16384 (strong), 32768 (paranoid)",
  "poly_modulus_degree": 8192,
  
  "comment_scale": "CKKS initial scale. 40-bits is a good default.",
  "ckks_scale_bits": 40,

  "comment_clients": "Number of smart meters to simulate in the test harness.",
  "num_clients": 10,

  "comment_paths": "File paths for keys and data.",
  "public_key_file": "keys/public_key.seal",
  "secret_key_file": "keys/secret_key.seal",
  "relin_keys_file": "keys/relin_keys.seal",
  "data_path_prefix": "data/ct_client_"
}


poly_modulus_degree: The main security parameter. Higher values (e.g., 16384) increase security and precision but increase computation time and ciphertext size.

ckks_scale_bits: Controls the precision of the real-number arithmetic.

num_clients: Controls how many distinct client simulations the test harness runs.

# Performance Metrics

The framework automatically logs the following metrics to the console for every run, allowing for rigorous performance analysis:

Key Generation Time: Time taken by KGC to generate keys.

Encryption Time: Time taken by a Client to encode and encrypt a single double.

Serialization Overhead: Time taken to save the ciphertext to disk.

Ciphertext Size: The storage footprint of the encrypted data (simulating bandwidth).

Aggregation Time: Time taken by the Aggregator to sum all client data.

Deserialization Overhead: Time taken by the Aggregator to load the ciphertexts.

Decryption Time: Time taken by the Control Center to decrypt the final result.

Precision Error: The difference between the expected plaintext result and the decrypted (approximate) result.

# Data Source

The simulation supports real-world data integration. It is designed to encrypt double (real number) values, making it compatible with the kWh readings from the Smart Meters in London dataset.

Source: Kaggle - Smart Meters in London

# License

This project is built for academic research and benchmarking purposes.
Uses Microsoft SEAL (MIT License).