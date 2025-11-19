#include <iostream>
#include <chrono>
#include <fstream>
#include <filesystem>
#include "seal/seal.h"
#include "json.hpp"
#include "network_utils.h"

using namespace std;
using namespace seal;
using json = nlohmann::json;
namespace fs = std::filesystem;

int main() {
    auto start_time = chrono::high_resolution_clock::now();
    
    cout << "=== Aggregator Node (Blind Processor) ===" << endl;
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
    string public_key_file = config["public_key_file"];
    string relin_keys_file = config["relin_keys_file"];
    string data_path_prefix = config["data_path_prefix"];
    
    // Set up CKKS encryption parameters (must match keygen and client)
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
    
    // Load relinearization keys
    cout << "Loading relinearization keys..." << endl;
    ifstream rk_file(relin_keys_file, ios::binary);
    if (!rk_file.is_open()) {
        cerr << "Error: Could not open relinearization keys file. Run keygen first." << endl;
        return 1;
    }
    
    RelinKeys relin_keys;
    relin_keys.load(context, rk_file);
    rk_file.close();
    
    // Initialize evaluator for homomorphic operations
    Evaluator evaluator(context);
    
    cout << "Cryptographic components loaded (no secret key - privacy preserved)" << endl;
    
    // Find all client data files
    vector<string> data_files;
    cout << "Scanning for client data files..." << endl;
    
    try {
        for (const auto& entry : fs::directory_iterator("data/")) {
            if (entry.is_regular_file()) {
                string filename = entry.path().filename().string();
                if (filename.find("ct_client_") == 0 && 
                    filename.length() >= 5 && 
                    filename.substr(filename.length() - 5) == ".seal") {
                    data_files.push_back(entry.path().string());
                }
            }
        }
    } catch (const fs::filesystem_error& ex) {
        cerr << "Error reading data directory: " << ex.what() << endl;
        return 1;
    }
    
    if (data_files.empty()) {
        cerr << "Error: No client data files found. Run clients first." << endl;
        return 1;
    }
    
    sort(data_files.begin(), data_files.end());
    cout << "Found " << data_files.size() << " client data files" << endl;
    
    auto load_start = chrono::high_resolution_clock::now();
    
    // Load all client ciphertexts
    vector<Ciphertext> client_ciphertexts;
    cout << "Loading client ciphertexts..." << endl;
    
    for (const string& filename : data_files) {
        ifstream data_file(filename, ios::binary);
        if (!data_file.is_open()) {
            cerr << "Warning: Could not open " << filename << endl;
            continue;
        }
        
        Ciphertext ct;
        ct.load(context, data_file);
        data_file.close();
        
        client_ciphertexts.push_back(ct);
        cout << "  Loaded: " << filename << endl;
    }
    
    auto load_end = chrono::high_resolution_clock::now();
    
    if (client_ciphertexts.empty()) {
        cerr << "Error: No valid ciphertexts loaded" << endl;
        return 1;
    }
    
    cout << "Successfully loaded " << client_ciphertexts.size() << " ciphertexts" << endl;
    
    auto aggregate_start = chrono::high_resolution_clock::now();
    
    // Perform homomorphic aggregation (summation)
    cout << "Performing homomorphic aggregation..." << endl;
    Ciphertext aggregated_result = client_ciphertexts[0];
    
    for (size_t i = 1; i < client_ciphertexts.size(); i++) {
        evaluator.add_inplace(aggregated_result, client_ciphertexts[i]);
        cout << "  Added client " << (i + 1) << " to aggregation" << endl;
    }
    
    auto aggregate_end = chrono::high_resolution_clock::now();
    
    // Serialize aggregated result for network transmission
    cout << "Serializing aggregated result for network transmission..." << endl;
    stringstream result_stream;
    aggregated_result.save(result_stream);
    string serialized_result = result_stream.str();
    
    cout << "Serialized result size: " << serialized_result.size() << " bytes" << endl;
    
    // Load network configuration
    string cc_host = config["control_center"]["host"];
    uint16_t cc_port = config["control_center"]["port"];
    string agg_cert_file = config["aggregator"]["certificate"];
    
    cout << "Connecting to Control Center at " << cc_host << ":" << cc_port << endl;
    
    // Load aggregator certificate
    NodeCertificate agg_cert;
    if (!NetworkUtils::load_certificate(agg_cert_file, agg_cert)) {
        cerr << "Error: Could not load aggregator certificate: " << agg_cert_file << endl;
        return 1;
    }
    
    cout << "Loaded aggregator certificate: " << agg_cert.node_id << endl;
    
    // Establish secure connection to Control Center
    auto network_start = chrono::high_resolution_clock::now();
    
    int sockfd = NetworkUtils::create_client_socket(cc_host, cc_port);
    if (sockfd < 0) {
        cerr << "Error: Failed to connect to Control Center" << endl;
        return 1;
    }
    
    NetworkUtils::set_socket_timeout(sockfd, config["aggregator"]["connect_timeout"]);
    
    SecureConnection secure_conn(sockfd);
    
    cout << "Authenticating with Control Center..." << endl;
    if (!secure_conn.authenticate_as_client(agg_cert)) {
        cerr << "Error: Failed to authenticate with Control Center" << endl;
        return 1;
    }
    
    cout << "✓ Authentication successful!" << endl;
    
    // Send aggregated encrypted data securely
    cout << "Transmitting encrypted aggregated data..." << endl;
    if (!secure_conn.send_secure_data(serialized_result.data(), serialized_result.size())) {
        cerr << "Error: Failed to send aggregated data" << endl;
        return 1;
    }
    
    cout << "✓ Data transmission successful!" << endl;
    
    auto network_end = chrono::high_resolution_clock::now();
    
    auto end_time = chrono::high_resolution_clock::now();
    
    // Performance metrics
    auto load_duration = chrono::duration_cast<chrono::milliseconds>(load_end - load_start);
    auto aggregate_duration = chrono::duration_cast<chrono::milliseconds>(aggregate_end - aggregate_start);
    auto network_duration = chrono::duration_cast<chrono::milliseconds>(network_end - network_start);
    auto total_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "=== Aggregation Complete ===" << endl;
    cout << "Clients processed: " << client_ciphertexts.size() << endl;
    cout << "Deserialization time: " << load_duration.count() << " ms" << endl;
    cout << "Homomorphic aggregation time: " << aggregate_duration.count() << " ms" << endl;
    cout << "Network transmission time: " << network_duration.count() << " ms" << endl;
    cout << "Total execution time: " << total_duration.count() << " ms" << endl;
    cout << "Data transmitted securely to Control Center via TCP/IP" << endl;
    cout << "PRIVACY PRESERVED: Aggregator never saw plaintext data!" << endl;
    cout << "NETWORK SECURITY: Authenticated connection with certificate validation" << endl;
    
    return 0;
}