#include <iostream>
#include <chrono>
#include <fstream>
#include <random>
#include <iomanip>
#include <thread>
#include <signal.h>
#include <sstream>
#include "seal/seal.h"
#include "json.hpp"
#include "network_utils.h"

using namespace std;
using namespace seal;
using json = nlohmann::json;

// Global flag for graceful shutdown
volatile bool running = true;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        cout << "\nReceived shutdown signal. Gracefully shutting down smart meter..." << endl;
        running = false;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <meter_id>" << endl;
        cerr << "Example: " << argv[0] << " 1" << endl;
        return 1;
    }
    
    // Setup signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    int meter_id = stoi(argv[1]);
    auto start_time = chrono::high_resolution_clock::now();
    
    cout << "=== Smart Meter Server " << meter_id << " ===" << endl;
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
    string cert_prefix = config["smart_meters"]["certificate_prefix"];
    uint16_t base_port = config["smart_meters"]["base_port"];
    int server_timeout = config["smart_meters"]["server_timeout"];
    
    // Calculate port for this meter
    uint16_t meter_port = base_port + meter_id;
    string cert_file = cert_prefix + to_string(meter_id) + ".cert";
    
    cout << "Meter ID: " << meter_id << ", Port: " << meter_port << endl;
    
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
    
    // Load smart meter certificate
    NodeCertificate meter_cert;
    if (!NetworkUtils::load_certificate(cert_file, meter_cert)) {
        cerr << "Error: Could not load smart meter certificate: " << cert_file << endl;
        return 1;
    }
    
    cout << "Loaded smart meter certificate: " << meter_cert.node_id << endl;
    
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
    
    // Serialize encrypted data for network transmission
    stringstream encrypted_stream;
    encrypted.save(encrypted_stream);
    string serialized_data = encrypted_stream.str();
    
    cout << "Encrypted data size: " << serialized_data.size() << " bytes" << endl;
    
    // Start TCP server for this smart meter
    cout << "Starting smart meter server on port " << meter_port << "..." << endl;
    int server_sockfd = NetworkUtils::create_server_socket(meter_port);
    if (server_sockfd < 0) {
        cerr << "Error: Failed to create smart meter server on port " << meter_port << endl;
        return 1;
    }
    
    NetworkUtils::set_socket_timeout(server_sockfd, server_timeout);
    cout << "✓ Smart meter server listening on port " << meter_port << endl;
    cout << "Waiting for aggregator connection..." << endl;
    
    while (running) {
        // Accept connection from aggregator
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_len);
        if (client_sockfd < 0) {
            if (running) {
                cerr << "Error accepting connection (may be timeout): " << NetworkUtils::get_last_error() << endl;
            }
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        cout << "✓ Accepted connection from aggregator: " << client_ip << ":" << ntohs(client_addr.sin_port) << endl;
        
        // Authenticate the aggregator
        SecureConnection secure_conn(client_sockfd);
        cout << "Authenticating aggregator..." << endl;
        
        if (!secure_conn.authenticate_as_server(meter_cert)) {
            cerr << "Error: Aggregator authentication failed" << endl;
            continue;
        }
        
        cout << "✓ Aggregator authenticated: " << secure_conn.get_peer_certificate().node_id << endl;
        
        auto network_start = chrono::high_resolution_clock::now();
        
        // Send encrypted data to aggregator
        cout << "Transmitting encrypted energy data..." << endl;
        if (!secure_conn.send_secure_data(serialized_data.data(), serialized_data.size())) {
            cerr << "Error: Failed to send encrypted data to aggregator" << endl;
            continue;
        }
        
        auto network_end = chrono::high_resolution_clock::now();
        
        cout << "✓ Data transmission successful!" << endl;
        
        // Close this connection and wait for next aggregator request
        secure_conn.close();
        
        auto session_duration = chrono::duration_cast<chrono::milliseconds>(network_end - network_start);
        cout << "Session completed in " << session_duration.count() << " ms" << endl;
        
        // For this demo, serve one request and exit
        // In production, the meter would continue serving requests
        break;
    }
    
    close(server_sockfd);
    
    auto end_time = chrono::high_resolution_clock::now();
    
    // Performance metrics
    auto encrypt_duration = chrono::duration_cast<chrono::microseconds>(encrypt_end - encrypt_start);
    auto total_duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    
    cout << "=== Smart Meter Server " << meter_id << " Complete ===" << endl;
    cout << "Original data: " << energy_consumption << " kWh" << endl;
    cout << "Encryption time: " << encrypt_duration.count() << " μs" << endl;
    cout << "Encrypted data size: " << serialized_data.size() << " bytes" << endl;
    cout << "Total execution time: " << total_duration.count() << " ms" << endl;
    cout << "Smart meter served data via TCP/IP on port " << meter_port << endl;
    cout << "PRIVACY PRESERVED: Data encrypted before network transmission" << endl;
    cout << "NETWORK READY: Realistic smart meter simulation complete" << endl;
    
    return 0;
}