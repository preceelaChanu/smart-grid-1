#include <iostream>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <thread>
#include <vector>
#include <future>
#include <mutex>
#include "seal/seal.h"
#include "json.hpp"
#include "network_utils.h"
#include "kdc_client.h"

using namespace std;
using namespace seal;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Thread-safe data collection
mutex ciphertext_mutex;
vector<Ciphertext> collected_ciphertexts;
vector<string> collection_errors;

// Function to collect data from a single smart meter
bool collect_from_smart_meter(int meter_id, const SEALContext& context, const NodeCertificate& agg_cert, const json& config) {
    try {
        string meter_host = "127.0.0.1"; // Assuming all meters run locally for demo
        uint16_t base_port = config["smart_meters"]["base_port"];
        uint16_t meter_port = base_port + meter_id;
        int connect_timeout = config["aggregator"]["connect_timeout"];
        
        cout << "Connecting to Smart Meter " << meter_id << " on port " << meter_port << "..." << endl;
        
        // Connect to smart meter
        int sockfd = NetworkUtils::create_client_socket(meter_host, meter_port);
        if (sockfd < 0) {
            string error = "Failed to connect to Smart Meter " + to_string(meter_id) + " on port " + to_string(meter_port);
            lock_guard<mutex> lock(ciphertext_mutex);
            collection_errors.push_back(error);
            return false;
        }
        
        NetworkUtils::set_socket_timeout(sockfd, connect_timeout);
        SecureConnection secure_conn(sockfd);
        
        // Authenticate with smart meter
        if (!secure_conn.authenticate_as_client(agg_cert)) {
            string error = "Authentication failed with Smart Meter " + to_string(meter_id);
            lock_guard<mutex> lock(ciphertext_mutex);
            collection_errors.push_back(error);
            return false;
        }
        
        cout << "✓ Connected and authenticated with Smart Meter " << meter_id << endl;
        
        // Receive encrypted data
        vector<uint8_t> encrypted_data;
        if (!secure_conn.receive_secure_data(encrypted_data)) {
            string error = "Failed to receive data from Smart Meter " + to_string(meter_id);
            lock_guard<mutex> lock(ciphertext_mutex);
            collection_errors.push_back(error);
            return false;
        }
        
        cout << "✓ Received " << encrypted_data.size() << " bytes from Smart Meter " << meter_id << endl;
        
        // Deserialize ciphertext
        stringstream data_stream;
        data_stream.write(reinterpret_cast<char*>(encrypted_data.data()), encrypted_data.size());
        
        Ciphertext meter_ciphertext;
        meter_ciphertext.load(context, data_stream);
        
        // Add to collection (thread-safe)
        lock_guard<mutex> lock(ciphertext_mutex);
        collected_ciphertexts.push_back(meter_ciphertext);
        
        cout << "✓ Successfully collected data from Smart Meter " << meter_id << endl;
        return true;
        
    } catch (const exception& e) {
        string error = "Exception collecting from Smart Meter " + to_string(meter_id) + ": " + e.what();
        lock_guard<mutex> lock(ciphertext_mutex);
        collection_errors.push_back(error);
        return false;
    }
}

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
    string agg_cert_file = config["aggregator"]["certificate"];
    int num_clients = config["num_clients"];
    int max_parallel = config["aggregator"]["max_parallel_connections"];
    
    cout << "Target smart meters: " << num_clients << endl;
    cout << "Max parallel connections: " << max_parallel << endl;
    
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
    
    // Load aggregator certificate first (needed for KDC authentication)
    NodeCertificate agg_cert;
    if (!NetworkUtils::load_certificate(agg_cert_file, agg_cert)) {
        cerr << "Error: Could not load aggregator certificate: " << agg_cert_file << endl;
        return 1;
    }
    
    cout << "Loaded aggregator certificate: " << agg_cert.node_id << endl;
    
    // Request keys from KDC
    cout << "Requesting cryptographic keys from KDC..." << endl;
    string kdc_host = config["key_distribution_center"]["host"];
    uint16_t kdc_port = config["key_distribution_center"]["port"];
    
    KDCClient kdc_client(kdc_host, kdc_port, agg_cert);
    
    PublicKey public_key;
    RelinKeys relin_keys;
    auto context_ptr = make_shared<SEALContext>(context);
    
    if (!kdc_client.request_public_keys(context_ptr, public_key, relin_keys)) {
        cerr << "Error: Failed to obtain keys from KDC. Falling back to file-based keys..." << endl;
        
        // Fallback to file-based key loading
        cout << "Loading public key..." << endl;
        ifstream pk_file(public_key_file, ios::binary);
        if (!pk_file.is_open()) {
            cerr << "Error: Could not open public key file and KDC request failed." << endl;
            return 1;
        }
        
        public_key.load(context, pk_file);
        pk_file.close();
        
        cout << "Loading relinearization keys..." << endl;
        ifstream rk_file(relin_keys_file, ios::binary);
        if (!rk_file.is_open()) {
            cerr << "Error: Could not open relinearization keys file and KDC request failed." << endl;
            return 1;
        }
        
        relin_keys.load(context, rk_file);
        rk_file.close();
        cout << "Loaded keys from files (fallback mode)" << endl;
    } else {
        cout << "✓ Successfully obtained keys from KDC" << endl;
    }
    
    // Initialize evaluator for homomorphic operations
    Evaluator evaluator(context);
    
    cout << "Cryptographic components loaded (no secret key - privacy preserved)" << endl;
    
    // Collect data from all smart meters using parallel connections
    cout << "Collecting data from " << num_clients << " smart meters..." << endl;
    auto collect_start = chrono::high_resolution_clock::now();
    
    // Create worker threads for parallel data collection
    vector<future<bool>> collection_futures;
    int active_connections = 0;
    
    for (int i = 1; i <= num_clients; i++) {
        // Limit concurrent connections
        while (active_connections >= max_parallel) {
            // Check if any connections completed
            for (auto it = collection_futures.begin(); it != collection_futures.end(); ) {
                if (it->wait_for(chrono::milliseconds(10)) == future_status::ready) {
                    it->get(); // Get result and handle any exceptions
                    it = collection_futures.erase(it);
                    active_connections--;
                } else {
                    ++it;
                }
            }
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        
        // Start new connection
        collection_futures.push_back(
            async(launch::async, collect_from_smart_meter, i, ref(context), ref(agg_cert), ref(config))
        );
        active_connections++;
    }
    
    // Wait for all collections to complete
    for (auto& future : collection_futures) {
        future.get();
    }
    
    auto collect_end = chrono::high_resolution_clock::now();
    
    // Report collection results
    cout << "Successfully collected from " << collected_ciphertexts.size() << " smart meters" << endl;
    if (!collection_errors.empty()) {
        cout << "Encountered " << collection_errors.size() << " errors:" << endl;
        for (const auto& error : collection_errors) {
            cout << "  - " << error << endl;
        }
    }
    
    if (collected_ciphertexts.empty()) {
        cerr << "Error: No data collected from smart meters" << endl;
        return 1;
    }
    
    auto aggregate_start = chrono::high_resolution_clock::now();
    
    // Perform homomorphic aggregation (summation)
    cout << "Performing homomorphic aggregation on " << collected_ciphertexts.size() << " ciphertexts..." << endl;
    Ciphertext aggregated_result = collected_ciphertexts[0];
    
    for (size_t i = 1; i < collected_ciphertexts.size(); i++) {
        evaluator.add_inplace(aggregated_result, collected_ciphertexts[i]);
        cout << "  Added Smart Meter " << (i + 1) << " to aggregation" << endl;
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
    
    cout << "Connecting to Control Center at " << cc_host << ":" << cc_port << endl;
    
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
    auto collect_duration = chrono::duration_cast<chrono::milliseconds>(collect_end - collect_start);
    auto aggregate_duration = chrono::duration_cast<chrono::milliseconds>(aggregate_end - aggregate_start);
    auto network_duration = chrono::duration_cast<chrono::milliseconds>(network_end - network_start);
    auto total_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "=== Networked Aggregation Complete ===" << endl;
    cout << "Smart meters contacted: " << num_clients << endl;
    cout << "Smart meters successfully collected: " << collected_ciphertexts.size() << endl;
    cout << "Collection failures: " << collection_errors.size() << endl;
    cout << "Data collection time: " << collect_duration.count() << " ms" << endl;
    cout << "Homomorphic aggregation time: " << aggregate_duration.count() << " ms" << endl;
    cout << "Network transmission time: " << network_duration.count() << " ms" << endl;
    cout << "Total execution time: " << total_duration.count() << " ms" << endl;
    cout << "Data transmitted securely to Control Center via TCP/IP" << endl;
    cout << "PRIVACY PRESERVED: Aggregator never saw plaintext data!" << endl;
    cout << "NETWORK SECURITY: Authenticated connections with all nodes" << endl;
    cout << "SCALABLE ARCHITECTURE: Supports distributed smart meter deployment" << endl;
    
    return 0;
}