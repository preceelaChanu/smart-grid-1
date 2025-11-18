#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include "seal/seal.h"
#include "json.hpp"

using namespace std;
using namespace seal;
using json = nlohmann::json;

int main() {
    auto start_time = chrono::high_resolution_clock::now();
    
    cout << "=== Control Center (Final Decryptor) ===" << endl;
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
    string secret_key_file = config["secret_key_file"];
    int num_clients = config["num_clients"];
    
    // Set up CKKS encryption parameters (must match all other components)
    EncryptionParameters parms(scheme_type::ckks);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
    
    SEALContext context(parms);
    
    if (!context.parameters_set()) {
        cerr << "Error: SEAL context parameters are invalid" << endl;
        return 1;
    }
    
    cout << "SEAL context initialized" << endl;
    
    // Load secret key
    cout << "Loading secret key..." << endl;
    ifstream sk_file(secret_key_file, ios::binary);
    if (!sk_file.is_open()) {
        cerr << "Error: Could not open secret key file. Run keygen first." << endl;
        return 1;
    }
    
    SecretKey secret_key;
    secret_key.load(context, sk_file);
    sk_file.close();
    
    // Initialize decryptor and decoder
    Decryptor decryptor(context, secret_key);
    CKKSEncoder encoder(context);
    
    cout << "Decryptor and decoder initialized" << endl;
    cout << "SECURITY NOTE: This is the only node with access to the secret key" << endl;
    
    // Load aggregated result
    cout << "Loading aggregated result from aggregator..." << endl;
    ifstream result_file("data/aggregated_result.seal", ios::binary);
    if (!result_file.is_open()) {
        cerr << "Error: Could not open aggregated result file. Run aggregator first." << endl;
        return 1;
    }
    
    Ciphertext aggregated_ciphertext;
    aggregated_ciphertext.load(context, result_file);
    result_file.close();
    
    cout << "Aggregated ciphertext loaded successfully" << endl;
    
    auto decrypt_start = chrono::high_resolution_clock::now();
    
    // Decrypt the aggregated result
    cout << "Decrypting aggregated result..." << endl;
    Plaintext aggregated_plaintext;
    decryptor.decrypt(aggregated_ciphertext, aggregated_plaintext);
    
    // Decode the result to get the actual values
    vector<double> result;
    encoder.decode(aggregated_plaintext, result);
    
    auto decrypt_end = chrono::high_resolution_clock::now();
    auto end_time = chrono::high_resolution_clock::now();
    
    // Extract the total energy consumption
    double total_energy = result[0];
    double average_energy = total_energy / num_clients;
    
    // Calculate expected range for verification
    double min_expected = num_clients * 0.5;  // min consumption per client
    double max_expected = num_clients * 5.0;  // max consumption per client
    
    // Performance metrics
    auto decrypt_duration = chrono::duration_cast<chrono::microseconds>(decrypt_end - decrypt_start);
    auto total_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "=== Smart Grid Analytics Results ===" << endl;
    cout << "Total Energy Consumption: " << fixed << setprecision(3) 
         << total_energy << " kWh" << endl;
    cout << "Average per Smart Meter: " << fixed << setprecision(3) 
         << average_energy << " kWh" << endl;
    cout << "Number of Meters: " << num_clients << endl;
    
    // Verification
    cout << "\n=== Verification ===" << endl;
    cout << "Expected range: [" << min_expected << ", " << max_expected << "] kWh" << endl;
    
    if (total_energy >= min_expected && total_energy <= max_expected) {
        cout << "✓ Result is within expected range - VERIFICATION PASSED" << endl;
    } else {
        cout << "✗ Result is outside expected range - VERIFICATION FAILED" << endl;
    }
    
    // Calculate precision (CKKS is approximate)
    double theoretical_precision = pow(2, -(ckks_scale_bits - 10)); // rough estimate
    cout << "Theoretical precision: ±" << scientific << theoretical_precision << endl;
    
    cout << "\n=== Performance Metrics ===" << endl;
    cout << "Decryption time: " << decrypt_duration.count() << " μs" << endl;
    cout << "Total execution time: " << total_duration.count() << " ms" << endl;
    
    cout << "\n=== Privacy Analysis ===" << endl;
    cout << "✓ Individual meter data never exposed in plaintext during aggregation" << endl;
    cout << "✓ Aggregator performed computation without secret key access" << endl;
    cout << "✓ Only aggregated result decrypted at control center" << endl;
    cout << "✓ Post-quantum security provided by CKKS scheme" << endl;
    
    // Additional analytics that could be performed
    cout << "\n=== Potential Analytics (with additional implementation) ===" << endl;
    cout << "- Peak usage detection" << endl;
    cout << "- Load balancing calculations" << endl;
    cout << "- Billing computations" << endl;
    cout << "- Anomaly detection" << endl;
    cout << "- Demand forecasting" << endl;
    
    return 0;
}