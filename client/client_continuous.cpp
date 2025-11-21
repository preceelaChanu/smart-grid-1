#include <iostream>
#include <chrono>
#include <fstream>
#include <random>
#include <iomanip>
#include <thread>
#include <signal.h>
#include <sstream>
#include <atomic>
#include "seal/seal.h"
#include "json.hpp"
#include "network_utils.h"
#include "kdc_client.h"

using namespace std;
using namespace seal;
using json = nlohmann::json;

// Global flag for graceful shutdown
atomic<bool> running(true);
atomic<bool> data_ready(false);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        cout << "\nReceived shutdown signal. Gracefully shutting down smart meter..." << endl;
        running = false;
    }
}

class SmartMeterContinuous {
private:
    int meter_id_;
    uint16_t port_;
    json config_;
    
    // SEAL components
    shared_ptr<SEALContext> context_;
    PublicKey public_key_;
    RelinKeys relin_keys_;
    Encryptor* encryptor_;
    CKKSEncoder* encoder_;
    double scale_;
    
    // Network components
    NodeCertificate meter_cert_;
    int server_sockfd_;
    
    // Data generation
    mt19937 rng_;
    uniform_real_distribution<double> energy_dist_;
    
    // Timing
    chrono::seconds transmission_interval_;
    chrono::steady_clock::time_point last_transmission_;
    
    // Current encrypted data
    vector<uint8_t> current_encrypted_data_;
    double current_energy_value_;
    mutex data_mutex_;
    
public:
    SmartMeterContinuous(int meter_id) : 
        meter_id_(meter_id),
        rng_(chrono::steady_clock::now().time_since_epoch().count()),
        energy_dist_(0.5, 5.0),  // Energy consumption between 0.5 and 5.0 kWh
        transmission_interval_(300)  // Send data every 5 minutes (300 seconds)
    {
        port_ = 9000 + meter_id_;
    }
    
    ~SmartMeterContinuous() {
        cleanup();
    }
    
    bool initialize() {
        cout << "=== Continuous Smart Meter Server " << meter_id_ << " ===" << endl;
        cout << "Initializing for continuous operation..." << endl;
        
        // Load configuration
        ifstream config_file("config.json");
        if (!config_file.is_open()) {
            cerr << "Error: Could not open config.json" << endl;
            return false;
        }
        
        config_file >> config_;
        config_file.close();
        
        // Initialize SEAL context
        size_t poly_modulus_degree = config_["poly_modulus_degree"];
        int ckks_scale_bits = config_["ckks_scale_bits"];
        
        EncryptionParameters parms(scheme_type::ckks);
        parms.set_poly_modulus_degree(poly_modulus_degree);
        parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
        
        context_ = make_shared<SEALContext>(parms);
        if (!context_->parameters_set()) {
            cerr << "Error: SEAL context parameters are invalid" << endl;
            return false;
        }
        
        cout << "SEAL context initialized" << endl;
        
        // Load certificate
        string cert_prefix = config_["smart_meters"]["certificate_prefix"].get<string>();
        string cert_file = cert_prefix + to_string(meter_id_) + ".cert";        if (!NetworkUtils::load_certificate(cert_file, meter_cert_)) {
            cerr << "Error: Could not load smart meter certificate: " << cert_file << endl;
            return false;
        }
        
        cout << "Loaded certificate: " << meter_cert_.node_id << endl;
        
        // Request keys from KDC
        if (!request_keys_from_kdc()) {
            cerr << "Failed to obtain keys from KDC, falling back to file keys" << endl;
            if (!load_fallback_keys()) {
                return false;
            }
        }
        
        // Initialize encryptor and encoder
        encryptor_ = new Encryptor(*context_, public_key_);
        encoder_ = new CKKSEncoder(*context_);
        scale_ = pow(2.0, ckks_scale_bits);
        
        cout << "Cryptographic components initialized" << endl;
        
        // Setup network server
        server_sockfd_ = NetworkUtils::create_server_socket(port_);
        if (server_sockfd_ < 0) {
            cerr << "Error: Failed to create server socket on port " << port_ << endl;
            return false;
        }
        
        cout << "✓ Smart meter server ready on port " << port_ << endl;
        cout << "Data transmission interval: " << transmission_interval_.count() << " seconds" << endl;
        
        last_transmission_ = chrono::steady_clock::now();
        
        return true;
    }
    
    void run_continuous() {
        cout << "Starting continuous smart meter operation..." << endl;
        cout << "Press Ctrl+C to stop gracefully" << endl;
        
        thread data_generator(&SmartMeterContinuous::data_generation_loop, this);
        thread network_server(&SmartMeterContinuous::network_server_loop, this);
        
        // Main monitoring loop
        while (running) {
            this_thread::sleep_for(chrono::seconds(1));
            
            // Check if it's time for status update
            auto now = chrono::steady_clock::now();
            auto time_since_last = chrono::duration_cast<chrono::minutes>(now - last_transmission_);
            
            if (time_since_last.count() > 0 && time_since_last.count() % 5 == 0) {
                print_status();
            }
        }
        
        // Wait for threads to complete
        if (data_generator.joinable()) data_generator.join();
        if (network_server.joinable()) network_server.join();
        
        cout << "Smart meter " << meter_id_ << " shutdown complete" << endl;
    }

private:
    bool request_keys_from_kdc() {
        string kdc_host = config_["key_distribution_center"]["host"];
        uint16_t kdc_port = config_["key_distribution_center"]["port"];
        
        KDCClient kdc_client(kdc_host, kdc_port, meter_cert_);
        
        if (kdc_client.request_public_keys(context_, public_key_, relin_keys_)) {
            cout << "✓ Successfully obtained keys from KDC" << endl;
            return true;
        }
        return false;
    }
    
    bool load_fallback_keys() {
        string public_key_file = config_["public_key_file"];
        
        ifstream pk_file(public_key_file, ios::binary);
        if (!pk_file.is_open()) {
            cerr << "Error: Could not open public key file" << endl;
            return false;
        }
        
        public_key_.load(*context_, pk_file);
        pk_file.close();
        
        cout << "Loaded keys from fallback files" << endl;
        return true;
    }
    
    void data_generation_loop() {
        cout << "Data generation thread started" << endl;
        
        while (running) {
            auto now = chrono::steady_clock::now();
            
            // Check if it's time to generate new data
            if (now - last_transmission_ >= transmission_interval_) {
                generate_and_encrypt_data();
                last_transmission_ = now;
                data_ready = true;
            }
            
            // Check every 10 seconds
            this_thread::sleep_for(chrono::seconds(10));
        }
        
        cout << "Data generation thread stopped" << endl;
    }
    
    void generate_and_encrypt_data() {
        lock_guard<mutex> lock(data_mutex_);
        
        // Simulate realistic energy consumption with time-based variations
        auto now = chrono::system_clock::now();
        auto time_t = chrono::system_clock::to_time_t(now);
        auto tm = *localtime(&time_t);
        
        // Add time-of-day variations (higher consumption during peak hours)
        double time_factor = 1.0;
        if (tm.tm_hour >= 7 && tm.tm_hour <= 10) {
            time_factor = 1.5;  // Morning peak
        } else if (tm.tm_hour >= 17 && tm.tm_hour <= 22) {
            time_factor = 1.8;  // Evening peak
        } else if (tm.tm_hour >= 23 || tm.tm_hour <= 6) {
            time_factor = 0.6;  // Night time low consumption
        }
        
        // Generate energy value with realistic variations
        current_energy_value_ = energy_dist_(rng_) * time_factor;
        
        // Add some randomness for realistic simulation
        uniform_real_distribution<double> variation(-0.2, 0.2);
        current_energy_value_ += variation(rng_);
        current_energy_value_ = max(0.1, current_energy_value_);  // Minimum 0.1 kWh
        
        // Encrypt the data
        vector<double> values = {current_energy_value_};
        Plaintext plain;
        encoder_->encode(values, scale_, plain);
        
        Ciphertext encrypted;
        encryptor_->encrypt(plain, encrypted);
        
        // Serialize to bytes
        stringstream stream;
        encrypted.save(stream);
        string serialized = stream.str();
        current_encrypted_data_ = vector<uint8_t>(serialized.begin(), serialized.end());
        
        // Log the data generation (remove unused timestamp variable)
        cout << "[" << put_time(localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] "
             << "Meter " << meter_id_ << ": Generated " << fixed << setprecision(3) 
             << current_energy_value_ << " kWh (encrypted: " << current_encrypted_data_.size() 
             << " bytes)" << endl;
    }
    
    void network_server_loop() {
        cout << "Network server thread started on port " << port_ << endl;
        
        while (running) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            // Set socket to non-blocking for graceful shutdown
            NetworkUtils::set_socket_nonblocking(server_sockfd_, true);
            
            int client_sockfd = accept(server_sockfd_, (struct sockaddr*)&client_addr, &client_len);
            if (client_sockfd < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK && running) {
                    NetworkUtils::log_network_event("ACCEPT_ERROR", NetworkUtils::get_last_error());
                }
                this_thread::sleep_for(chrono::milliseconds(100));
                continue;
            }
            
            // Handle client connection
            thread client_thread(&SmartMeterContinuous::handle_aggregator_connection, this, client_sockfd, client_addr);
            client_thread.detach();
        }
        
        cout << "Network server thread stopped" << endl;
    }
    
    void handle_aggregator_connection(int client_sockfd, struct sockaddr_in client_addr) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        SecureConnection secure_conn(client_sockfd);
        
        // Authenticate aggregator
        if (!secure_conn.authenticate_as_server(meter_cert_)) {
            cerr << "Authentication failed from " << client_ip << endl;
            return;
        }
        
        string aggregator_id = secure_conn.get_peer_certificate().node_id;
        cout << "✓ Aggregator connected: " << aggregator_id << " from " << client_ip << endl;
        
        // Wait for current data to be available
        while (running && !data_ready) {
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        
        // Send the latest encrypted data
        {
            lock_guard<mutex> lock(data_mutex_);
            if (!current_encrypted_data_.empty()) {
                if (secure_conn.send_secure_data(current_encrypted_data_.data(), current_encrypted_data_.size())) {
                    cout << "✓ Sent " << current_encrypted_data_.size() << " bytes to " << aggregator_id << endl;
                } else {
                    cerr << "Failed to send data to " << aggregator_id << endl;
                }
            }
        }
        
        cout << "Session completed with " << aggregator_id << endl;
    }
    
    void print_status() {
        lock_guard<mutex> lock(data_mutex_);
        auto now = chrono::system_clock::now();
        auto time_t = chrono::system_clock::to_time_t(now);
        
        cout << "\n=== Smart Meter " << meter_id_ << " Status ===" << endl;
        cout << "Current time: " << put_time(localtime(&time_t), "%Y-%m-%d %H:%M:%S") << endl;
        cout << "Latest reading: " << fixed << setprecision(3) << current_energy_value_ << " kWh" << endl;
        cout << "Data ready: " << (data_ready ? "Yes" : "No") << endl;
        cout << "Port: " << port_ << endl;
        cout << "====================================" << endl;
    }
    
    void cleanup() {
        if (encryptor_) {
            delete encryptor_;
            encryptor_ = nullptr;
        }
        if (encoder_) {
            delete encoder_;
            encoder_ = nullptr;
        }
        if (server_sockfd_ >= 0) {
            close(server_sockfd_);
            server_sockfd_ = -1;
        }
    }
};

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
    
    SmartMeterContinuous meter(meter_id);
    
    if (!meter.initialize()) {
        cerr << "Failed to initialize smart meter" << endl;
        return 1;
    }
    
    meter.run_continuous();
    
    return 0;
}