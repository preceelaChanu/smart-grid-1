#include <iostream>
#include <chrono>
#include <fstream>
#include <random>
#include <iomanip>
#include "seal/seal.h"
#include "json.hpp"

using namespace std;
using namespace seal;
using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <client_id>" << endl;
        cerr << "Example: " << argv[0] << " 1" << endl;
        return 1;
    }
    
    int client_id = stoi(argv[1]);
    auto start_time = chrono::high_resolution_clock::now();
    
    cout << "=== Smart Meter Client " << client_id << " ===" << endl;
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
    string data_path_prefix = config["data_path_prefix"];
    
    // Set up CKKS encryption parameters (must match keygen)
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    
    SEALContext context(parms);
    
    if (!context.parameters_set()) {
        cerr << "Error: SEAL context parameters are invalid" << endl;
        return 1;
    }
    
    cout << "SEAL context initialized" << endl;
    
    // Load public key
    cout << "Loading public key..." << endl;
    ifstream pk_file(public_key_file, ios::binary);
    if (!pk_file.is_open()) {
        cerr << "Error: Could not open public key file. Run keygen first." << endl;
        return 1;
    }
    
    PublicKey public_key;
    public_key.load(context, pk_file);
    pk_file.close();
    
    // Initialize encryptor and encoder
    Encryptor encryptor(context, public_key);
    CKKSEncoder encoder(context);
    double scale = pow(2.0, ckks_scale_bits);
    
    cout << "Encryptor and encoder initialized (scale: 2^" << ckks_scale_bits << ")" << endl;
    
    // Generate realistic smart meter data (energy consumption in kWh)
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dist(0.5, 5.0); // Typical household consumption range
    
    double energy_consumption = dist(gen);
    
    cout << "Generated energy consumption: " << fixed << setprecision(3) 
         << energy_consumption << " kWh" << endl;
    
    auto encrypt_start = chrono::high_resolution_clock::now();
    
    // Encode the energy consumption data
    Plaintext plain;
    vector<double> input = {energy_consumption};
    encoder.encode(input, scale, plain);
    
    // Encrypt the encoded data
    Ciphertext encrypted;
    encryptor.encrypt(plain, encrypted);
    
    auto encrypt_end = chrono::high_resolution_clock::now();
    
    // Save encrypted data to file
    string output_filename = data_path_prefix + to_string(client_id) + ".seal";
    
    auto save_start = chrono::high_resolution_clock::now();
    
    ofstream data_file(output_filename, ios::binary);
    if (!data_file.is_open()) {
        cerr << "Error: Could not create data file " << output_filename << endl;
        return 1;
    }
    
    encrypted.save(data_file);
    data_file.close();
    
    auto save_end = chrono::high_resolution_clock::now();
    auto end_time = chrono::high_resolution_clock::now();
    
    // Calculate ciphertext size
    ifstream size_file(output_filename, ios::binary | ios::ate);
    streampos file_size = size_file.tellg();
    size_file.close();
    
    // Performance metrics
    auto encrypt_duration = chrono::duration_cast<chrono::microseconds>(encrypt_end - encrypt_start);
    auto save_duration = chrono::duration_cast<chrono::microseconds>(save_end - save_start);
    auto total_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "=== Client " << client_id << " Encryption Complete ===" << endl;
    cout << "Original data: " << energy_consumption << " kWh" << endl;
    cout << "Encryption time: " << encrypt_duration.count() << " μs" << endl;
    cout << "Serialization time: " << save_duration.count() << " μs" << endl;
    cout << "Ciphertext size: " << file_size << " bytes" << endl;
    cout << "Total execution time: " << total_duration.count() << " ms" << endl;
    cout << "Encrypted data saved to: " << output_filename << endl;
    cout << "Data ready for aggregation (no secret key exposure)" << endl;
    
    return 0;
}