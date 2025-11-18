#include <iostream>
#include <chrono>
#include <fstream>
#include "seal/seal.h"
#include "json.hpp"

using namespace std;
using namespace seal;
using json = nlohmann::json;

int main() {
    auto start_time = chrono::high_resolution_clock::now();
    
    cout << "=== Key Generation Center (KGC) ===" << endl;
    cout << "Loading configuration..." << endl;
    
    // Load configuration
    ifstream config_file("config.json");
    if (!config_file.is_open()) {
        cerr << "Error: Could not open config.json" << endl;
        return 1;
    }
    
    json config;
    config_file >> config;
    config_file.close();
    
    // Extract parameters
    size_t poly_modulus_degree = config["poly_modulus_degree"];
    int ckks_scale_bits = config["ckks_scale_bits"];
    string public_key_file = config["public_key_file"];
    string secret_key_file = config["secret_key_file"];
    string relin_keys_file = config["relin_keys_file"];
    
    cout << "Poly modulus degree: " << poly_modulus_degree << endl;
    cout << "CKKS scale bits: " << ckks_scale_bits << endl;
    
    // Set up CKKS encryption parameters
    EncryptionParameters parms(scheme_type::ckks);
    
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    
    // Initialize SEAL context
    SEALContext context(parms);
    
    // Validate context
    if (!context.parameters_set()) {
        cerr << "Error: SEAL context parameters are invalid" << endl;
        return 1;
    }
    
    cout << "SEAL context created successfully" << endl;
    
    auto keygen_start = chrono::high_resolution_clock::now();
    
    // Generate keys
    cout << "Generating cryptographic keys..." << endl;
    KeyGenerator keygen(context);
    
    SecretKey secret_key = keygen.secret_key();
    PublicKey public_key;
    keygen.create_public_key(public_key);
    
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    
    auto keygen_end = chrono::high_resolution_clock::now();
    
    // Save keys to files
    cout << "Saving keys to files..." << endl;
    
    // Save secret key
    ofstream sk_file(secret_key_file, ios::binary);
    if (!sk_file.is_open()) {
        cerr << "Error: Could not create secret key file" << endl;
        return 1;
    }
    secret_key.save(sk_file);
    sk_file.close();
    
    // Save public key
    ofstream pk_file(public_key_file, ios::binary);
    if (!pk_file.is_open()) {
        cerr << "Error: Could not create public key file" << endl;
        return 1;
    }
    public_key.save(pk_file);
    pk_file.close();
    
    // Save relinearization keys
    ofstream rk_file(relin_keys_file, ios::binary);
    if (!rk_file.is_open()) {
        cerr << "Error: Could not create relinearization keys file" << endl;
        return 1;
    }
    relin_keys.save(rk_file);
    rk_file.close();
    
    auto end_time = chrono::high_resolution_clock::now();
    
    // Performance metrics
    auto keygen_duration = chrono::duration_cast<chrono::milliseconds>(keygen_end - keygen_start);
    auto total_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "=== Key Generation Complete ===" << endl;
    cout << "Key generation time: " << keygen_duration.count() << " ms" << endl;
    cout << "Total execution time: " << total_duration.count() << " ms" << endl;
    cout << "Files created:" << endl;
    cout << "  - " << public_key_file << endl;
    cout << "  - " << secret_key_file << endl;
    cout << "  - " << relin_keys_file << endl;
    cout << "Keys are ready for use by clients, aggregator, and control center." << endl;
    
    return 0;
}