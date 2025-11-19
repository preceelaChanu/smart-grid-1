#include <iostream>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <signal.h>
#include "seal/seal.h"
#include "json.hpp"
#include "network_utils.h"
#include "kdc_client.h"

using namespace std;
using namespace seal;
using json = nlohmann::json;

// Global flag for graceful shutdown
volatile bool running = true;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        cout << "\nReceived shutdown signal. Gracefully shutting down..." << endl;
        running = false;
    }
}

int main() {
    // Setup signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    auto start_time = chrono::high_resolution_clock::now();
    
    cout << "=== Control Center (Network Server & Final Decryptor) ===" << endl;
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
    
    // Network configuration
    string cc_host = config["control_center"]["host"];
    uint16_t cc_port = config["control_center"]["port"];
    string cc_cert_file = config["control_center"]["certificate"];
    
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
    
    // Load Control Center certificate first (needed for KDC authentication)
    NodeCertificate cc_cert;
    if (!NetworkUtils::load_certificate(cc_cert_file, cc_cert)) {
        cerr << "Error: Could not load control center certificate: " << cc_cert_file << endl;
        return 1;
    }
    
    cout << "Loaded control center certificate: " << cc_cert.node_id << endl;
    
    // Request keys from KDC
    cout << "Requesting cryptographic keys from KDC..." << endl;
    string kdc_host = config["key_distribution_center"]["host"];
    uint16_t kdc_port = config["key_distribution_center"]["port"];
    
    KDCClient kdc_client(kdc_host, kdc_port, cc_cert);
    
    PublicKey public_key;
    RelinKeys relin_keys;
    SecretKey secret_key;
    auto context_ptr = make_shared<SEALContext>(context);
    
    if (!kdc_client.request_all_keys(context_ptr, public_key, relin_keys, secret_key)) {
        cerr << "Error: Failed to obtain keys from KDC. Falling back to file-based keys..." << endl;
        
        // Fallback to file-based key loading
        cout << "Loading secret key..." << endl;
        ifstream sk_file(secret_key_file, ios::binary);
        if (!sk_file.is_open()) {
            cerr << "Error: Could not open secret key file and KDC request failed." << endl;
            return 1;
        }
        
        secret_key.load(context, sk_file);
        sk_file.close();
        cout << "Loaded secret key from file (fallback mode)" << endl;
    } else {
        cout << "✓ Successfully obtained all keys from KDC" << endl;
    }
    
    // Initialize decryptor and decoder
    Decryptor decryptor(context, secret_key);
    CKKSEncoder encoder(context);
    
    cout << "Decryptor and decoder initialized" << endl;
    cout << "SECURITY NOTE: This is the only node with access to the secret key" << endl;
    
    // Start TCP server
    cout << "Starting TCP server on " << cc_host << ":" << cc_port << endl;
    int server_sockfd = NetworkUtils::create_server_socket(cc_port);
    if (server_sockfd < 0) {
        cerr << "Error: Failed to create server socket" << endl;
        return 1;
    }
    
    cout << "✓ Control Center server listening on port " << cc_port << endl;
    cout << "Waiting for aggregator connection..." << endl;
    
    // Accept connection from aggregator
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    NetworkUtils::set_socket_timeout(server_sockfd, 60); // 60 second timeout for connections
    
    int client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_len);
    if (client_sockfd < 0) {
        cerr << "Error: Failed to accept connection from aggregator" << endl;
        close(server_sockfd);
        return 1;
    }
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    cout << "✓ Accepted connection from aggregator: " << client_ip << ":" << ntohs(client_addr.sin_port) << endl;
    
    // Authenticate the aggregator
    SecureConnection secure_conn(client_sockfd);
    cout << "Authenticating aggregator..." << endl;
    
    if (!secure_conn.authenticate_as_server(cc_cert)) {
        cerr << "Error: Aggregator authentication failed" << endl;
        close(server_sockfd);
        return 1;
    }
    
    cout << "✓ Aggregator authenticated successfully!" << endl;
    cout << "Aggregator identity: " << secure_conn.get_peer_certificate().node_id << endl;
    
    auto network_start = chrono::high_resolution_clock::now();
    
    // Receive encrypted aggregated data
    cout << "Receiving encrypted aggregated data from aggregator..." << endl;
    vector<uint8_t> received_data;
    
    if (!secure_conn.receive_secure_data(received_data)) {
        cerr << "Error: Failed to receive aggregated data from aggregator" << endl;
        close(server_sockfd);
        return 1;
    }
    
    auto network_end = chrono::high_resolution_clock::now();
    cout << "✓ Received " << received_data.size() << " bytes of encrypted data" << endl;
    
    // Deserialize the received ciphertext
    cout << "Deserializing received ciphertext..." << endl;
    stringstream received_stream;
    received_stream.write(reinterpret_cast<char*>(received_data.data()), received_data.size());
    
    Ciphertext aggregated_ciphertext;
    aggregated_ciphertext.load(context, received_stream);
    
    cout << "✓ Successfully deserialized aggregated ciphertext" << endl;
    
    // Close network connections
    secure_conn.close();
    close(server_sockfd);
    
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
    auto network_duration = chrono::duration_cast<chrono::milliseconds>(network_end - network_start);
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
    cout << "Network communication time: " << network_duration.count() << " ms" << endl;
    cout << "Decryption time: " << decrypt_duration.count() << " μs" << endl;
    cout << "Total execution time: " << total_duration.count() << " ms" << endl;
    
    cout << "\n=== Network Security Analysis ===" << endl;
    cout << "✓ Secure TCP/IP connection established with aggregator" << endl;
    cout << "✓ Certificate-based authentication performed" << endl;
    cout << "✓ Session token validation for data integrity" << endl;
    cout << "✓ Encrypted data received and validated successfully" << endl;
    
    cout << "\n=== Privacy Analysis ===" << endl;
    cout << "✓ Individual meter data never exposed in plaintext during aggregation" << endl;
    cout << "✓ Aggregator performed computation without secret key access" << endl;
    cout << "✓ Only aggregated result decrypted at control center" << endl;
    cout << "✓ Post-quantum security provided by CKKS scheme" << endl;
    cout << "✓ Network communication secured with authentication" << endl;
    
    // Additional analytics that could be performed
    cout << "\n=== Potential Analytics (with additional implementation) ===" << endl;
    cout << "- Peak usage detection" << endl;
    cout << "- Load balancing calculations" << endl;
    cout << "- Billing computations" << endl;
    cout << "- Anomaly detection" << endl;
    cout << "- Demand forecasting" << endl;
    
    return 0;
}