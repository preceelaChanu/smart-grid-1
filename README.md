# Privacy-Preserving Smart Grid Analytics Framework

  

## Overview

This project is a real-time framework designed to benchmark the performance of **Homomorphic Encryption (HE)** algorithms within a Smart Grid environment. It simulates a realistic privacy-preserving architecture where Smart Meters (Clients) encrypt their real-number energy consumption data (e.g., `1.234 kWh`) before sending it to an Analytics Center (Server).

The system utilizes the **CKKS (Cheon-Kim-Kim-Song) scheme** from the Microsoft SEAL library. CKKS is a post-quantum, "levelled" HE scheme that supports approximate arithmetic on encrypted real numbers, making it ideal for smart grid analytics (averages, weighted sums, etc.).

## System Architecture

The project follows a trusted initialization, untrusted network model:

```mermaid
graph TD
    KGC[Key Generation Center] -- Generates Keys --> Keys[(Key Storage)]
    Keys -- Public Key --> Client[Smart Meter (Client)]
    Keys -- Public Key + Relin Keys --> Server[Analytics Server]
    Keys -- Secret Key --> Client
    Client -- Encrypts Data (double) --> Data[(Encrypted Data Files)]
    Data -- Loads Ciphertexts --> Server
    Server -- Computes (Analytics) --> Result[Encrypted Result]
```

1.  **KeyGen (KGC):** Generates Public, Private, and Relinearization keys based on security configurations.
2.  **Client (Smart Meter):** Simulates a smart meter. Reads real-world data (e.g., from the London Smart Meter dataset), encrypts the `double` value using the Public Key, and serializes the ciphertext.
3.  **Server (Analytics Center):** Loads encrypted data from multiple clients, performs homomorphic computation (e.g., aggregation, multiplication), and verifies the result without ever decrypting individual user inputs.

## Project Structure

```text
smart_grid_benchmark/
├── CMakeLists.txt       # Master build configuration
├── config.json          # Central configuration for security & simulation settings
├── run_test.sh          # Automated test harness script
├── include/             # External headers (nlohmann/json)
├── keygen/
│   └── keygen.cpp       # Source code for Key Generation Center (CKKS)
├── client/
│   └── client.cpp       # Source code for Smart Meter Simulator (CKKS)
├── server/
│   └── server.cpp       # Source code for Analytics Server (CKKS)
├── keys/                # Storage for generated keys (PK, SK, RLK)
└── data/                # Storage for encrypted smart meter data
```

## Prerequisites

Before running the framework, ensure your system has the following installed:

  * **OS:** Ubuntu Linux (20.04 or 22.04 recommended)
  * **Compiler:** `g++` (supporting C++17)
  * **Build System:** `cmake` (version 3.16+)
  * **Utilities:** `git`, `jq` (for JSON parsing in scripts)

<!-- end list -->

```bash
sudo apt update
sudo apt install build-essential g++ cmake git jq
```

## Installation

### 1\. Clone the Repository

```bash
mkdir smart_grid_benchmark
cd smart_grid_benchmark
# (If you are using git, clone here. Otherwise, create the folders manually)
```

### 2\. Install Microsoft SEAL (v4.0.0)

The project relies on the Microsoft SEAL library.

```bash
# Clone and build SEAL inside the project or externally
git clone https://github.com/microsoft/SEAL.git
cd SEAL
git checkout v4.0.0
cmake -S . -B build -DSEAL_THROW_ON_TRANSPARENT_CIPHERTEXT=OFF
cmake --build build
sudo cmake --install build
cd ..
```

### 3\. Install JSON Helper

The project uses `nlohmann/json` for configuration parsing.

```bash
mkdir -p include
wget -O include/json.hpp https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
```

### 4\. Build the Framework

```bash
mkdir build
cd build
cmake ..
make
cd ..
```

## Usage

You can run the components individually or use the automated test harness.

### Option A: Automated Test Harness (Recommended)

The `run_test.sh` script orchestrates the entire flow: it runs KeyGen once, simulates `N` clients (defined in config), and then runs the Server to aggregate the results.

```bash
chmod +x run_test.sh
./run_test.sh
```

### Option B: Manual Execution

1.  **Generate Keys:**
    ```bash
    ./build/keygen
    ```
2.  **Run Clients:** (Replace `1` with the Client ID)
    ```bash
    ./build/client 1
    ./build/client 2
    ```
3.  **Run Server:**
    ```bash
    ./build/server
    ```

## Configuration (`config.json`)

All experiment variables are controlled via `config.json`. You do not need to recompile the code to change these settings.

```json
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
```

  * **poly\_modulus\_degree:** Higher values (e.g., 16384) increase security and precision, but also increase computation time and ciphertext size.
  * **ckks\_scale\_bits:** Controls the precision of the real numbers.
  * **num\_clients:** Controls how many distinct client simulations the test harness runs.

## Performance Metrics

The framework automatically logs the following metrics to the console for every run:

  * **Key Generation Time:** Time taken to generate PK, SK, and RLK.
  * **Encryption Time:** Time taken for the client to encode and encrypt a single `double`.
  * **Serialization Overhead:** Time taken to save the ciphertext to disk.
  * **Ciphertext Size:** The storage footprint of the encrypted data (simulating bandwidth).
  * **Computation Time:** Time taken by the server to aggregate all client data (e.g., `add`, `multiply`).
  * **Deserialization Overhead:** Time taken by the server to load the ciphertexts.
  * **Decryption Time:** Time taken to decrypt the final result.
  * **Precision Error:** The difference between the expected plaintext result and the decrypted (approximate) result.

## Data Source

The simulation supports real-world data integration. It is designed to encrypt `double` (real number) values, making it perfect for the `kWh` readings from the **Smart Meters in London** dataset.

  * **Source:** [Kaggle - Smart Meters in London](https://www.kaggle.com/datasets/jeanmidev/smart-meters-in-london)

## License

This project is built for academic research and benchmarking purposes.
Uses **Microsoft SEAL** (MIT License).