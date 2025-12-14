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
#include "performance_metrics.h"

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
    
    // Data generation based on real dataset patterns
    mt19937 rng_;
    uniform_real_distribution<double> energy_dist_;
    
    // Household consumption profiles (based on dataset analysis)
    enum class HouseholdType {
        LOW_CONSUMER,      // 95.4% - avg <0.5 kWh
        MEDIUM_CONSUMER,   // 4.6% - avg 0.5-2.0 kWh  
        HIGH_CONSUMER,     // 0.0% - avg >2.0 kWh
        VARIABLE_CONSUMER  // 51.0% - high variation
    };
    
    HouseholdType household_type_;
    double base_consumption_;
    double variation_factor_;
    bool is_variable_consumer_;
    
    // Temporal consumption patterns (from dataset)
    static constexpr double hourly_factors_[24] = {
        0.517, 0.476, 0.448, 0.423, 0.516, 0.563, 0.610, 0.704,  // 0-7h
        0.751, 0.775, 0.798, 0.845, 0.892, 0.915, 0.939, 0.962,  // 8-15h  
        0.986, 1.056, 1.127, 1.479, 1.197, 1.169, 1.141, 0.587   // 16-23h
    };
    
    static constexpr double daily_factors_[7] = {
        1.042, 0.967, 0.972, 0.986, 0.993, 1.000, 1.042  // Sun-Sat
    };
    
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
        energy_dist_(0.0, 1.0),  // Uniform distribution for random factors
        transmission_interval_(20)  // Send data every 20 seconds (simulating 5 minutes)
    {
        port_ = 9000 + meter_id_;
        
        // Assign household type based on dataset statistics
        uniform_real_distribution<double> type_selector(0.0, 1.0);
        double type_rand = type_selector(rng_);
        
        if (type_rand < 0.954) {  // 95.4% low consumers
            household_type_ = HouseholdType::LOW_CONSUMER;
            base_consumption_ = 0.15 + (type_selector(rng_) * 0.35);  // 0.15-0.5 kWh
            variation_factor_ = 0.3;
        } else if (type_rand < 0.9995) {  // 4.6% medium consumers 
            household_type_ = HouseholdType::MEDIUM_CONSUMER;
            base_consumption_ = 0.5 + (type_selector(rng_) * 1.5);   // 0.5-2.0 kWh
            variation_factor_ = 0.4;
        } else {  // 0.05% high consumers (very rare)
            household_type_ = HouseholdType::HIGH_CONSUMER;
            base_consumption_ = 2.0 + (type_selector(rng_) * 0.112); // 2.0-2.112 kWh
            variation_factor_ = 0.2;
        }
        
        // 51% are variable consumers with higher variation
        is_variable_consumer_ = (type_selector(rng_) < 0.51);
        if (is_variable_consumer_) {
            variation_factor_ *= 2.0;  // Double the variation for variable consumers
        }
        
        // Adjust base consumption to match dataset average (0.213 kWh)
        // Apply a scaling factor to ensure overall average matches
        base_consumption_ *= 0.85;  // Scale down to match real average
    }
    
    ~SmartMeterContinuous() {
        cleanup();
    }
    
    bool initialize() {
        cout << "=== Continuous Smart Meter Server " << meter_id_ << " ===" << endl;
        cout << "Initializing for continuous operation..." << endl;
        
        // Load configuration
        // Try to load config from current directory first, then parent
        ifstream config_file("config.json");
        if (!config_file.is_open()) {
            config_file.open("../config.json");
        }
        if (!config_file.is_open()) {
            cerr << "Error: Could not open config.json in current or parent directory" << endl;
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
        cout << "Data transmission interval: " << transmission_interval_.count() << " seconds (15x accelerated - simulates 5min)" << endl;
        
        // Initialize performance metrics logging
        PerformanceMetrics::initializeCSVFiles();
        
        // Log initial security metrics
        PerformanceMetrics::SecurityMetrics sec_metrics;
        sec_metrics.algorithm = "CKKS";
        sec_metrics.key_size_bits = poly_modulus_degree * 40;  // Approximate
        sec_metrics.poly_modulus_degree = poly_modulus_degree;
        sec_metrics.security_level_bits = PerformanceMetrics::estimateSecurityLevel(poly_modulus_degree);
        sec_metrics.quantum_resistant = true;
        sec_metrics.attack_model = "IND-CPA";
        sec_metrics.ciphertext_expansion = 100;  // Will be updated with actual data
        sec_metrics.timestamp = PerformanceMetrics::getCurrentTimestamp();
        PerformanceMetrics::logSecurityMetrics(sec_metrics);
        
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
            
            // Check every 2 seconds (faster checking for 20-second cycles)
            this_thread::sleep_for(chrono::seconds(2));
        }
        
        cout << "Data generation thread stopped" << endl;
    }
    
    void generate_and_encrypt_data() {
        lock_guard<mutex> lock(data_mutex_);
        
        // Get current time for temporal pattern analysis
        auto now = chrono::system_clock::now();
        auto time_t = chrono::system_clock::to_time_t(now);
        auto tm = *localtime(&time_t);
        
        // Apply hourly consumption factor (from dataset peak analysis)
        double hourly_factor = hourly_factors_[tm.tm_hour];
        
        // Apply daily consumption factor (from dataset weekly pattern)
        double daily_factor = daily_factors_[tm.tm_wday];
        
        // Generate base consumption value
        double random_factor = energy_dist_(rng_);
        
        // Apply household-specific consumption pattern
        current_energy_value_ = base_consumption_ * hourly_factor * daily_factor;
        
        // Add variation based on household type and variability
        normal_distribution<double> variation(0.0, variation_factor_);
        double variation_amount = variation(rng_);
        current_energy_value_ += (current_energy_value_ * variation_amount);
        
        // Add small random noise to simulate meter precision
        uniform_real_distribution<double> noise(-0.001, 0.001);
        current_energy_value_ += noise(rng_);
        
        // Ensure non-negative consumption (some households have near-zero usage)
        current_energy_value_ = max(0.0, current_energy_value_);
        
        // Implement 0.000 kWh consumers (some households from dataset)
        if (household_type_ == HouseholdType::LOW_CONSUMER && random_factor < 0.02) {
            current_energy_value_ = 0.0;  // 2% chance of zero consumption
        }
        
        // Measure encryption performance
        auto encryption_start = chrono::high_resolution_clock::now();
        
        // Encrypt the data
        vector<double> values = {current_energy_value_};
        Plaintext plain;
        encoder_->encode(values, scale_, plain);
        
        Ciphertext encrypted;
        encryptor_->encrypt(plain, encrypted);
        
        auto encryption_end = chrono::high_resolution_clock::now();
        auto duration_ns = chrono::duration_cast<chrono::nanoseconds>(encryption_end - encryption_start);
        double encryption_time = duration_ns.count() / 1000000.0;  // Convert nanoseconds to milliseconds
        
        // Serialize to bytes
        stringstream stream;
        encrypted.save(stream);
        string serialized = stream.str();
        current_encrypted_data_ = vector<uint8_t>(serialized.begin(), serialized.end());
        
        // Log encryption performance metrics
        PerformanceMetrics::EncryptionMetrics enc_metrics;
        enc_metrics.algorithm = "CKKS";
        enc_metrics.poly_modulus_degree = context_->key_context_data()->parms().poly_modulus_degree();
        enc_metrics.scale_bits = static_cast<int>(log2(scale_));
        enc_metrics.plaintext_size_bytes = sizeof(double);  // Single double value
        enc_metrics.ciphertext_size_bytes = current_encrypted_data_.size();
        enc_metrics.encryption_time_ms = encryption_time;
        enc_metrics.communication_overhead = static_cast<double>(current_encrypted_data_.size()) / sizeof(double);
        enc_metrics.security_level_bits = PerformanceMetrics::estimateSecurityLevel(enc_metrics.poly_modulus_degree);
        enc_metrics.timestamp = PerformanceMetrics::getCurrentTimestamp();
        enc_metrics.meter_id = meter_id_;
        PerformanceMetrics::logEncryptionMetrics(enc_metrics);
        
        // Log the data generation with household type info
        string household_type_str;
        switch (household_type_) {
            case HouseholdType::LOW_CONSUMER: household_type_str = "Low"; break;
            case HouseholdType::MEDIUM_CONSUMER: household_type_str = "Medium"; break;
            case HouseholdType::HIGH_CONSUMER: household_type_str = "High"; break;
            case HouseholdType::VARIABLE_CONSUMER: household_type_str = "Variable"; break;
        }
        
        cout << "[" << put_time(localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] "
             << "Meter " << meter_id_ << " (" << household_type_str 
             << (is_variable_consumer_ ? "-Variable" : "") << "): " 
             << fixed << setprecision(3) << current_energy_value_ << " kWh "
             << "(H:" << fixed << setprecision(2) << hourly_factor 
             << ", D:" << daily_factor << ") "
             << "(encrypted: " << current_encrypted_data_.size() 
             << " bytes, " << fixed << setprecision(3) << encryption_time << " ms)" << endl;
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
                auto network_start = chrono::high_resolution_clock::now();
                bool success = secure_conn.send_secure_data(current_encrypted_data_.data(), current_encrypted_data_.size());
                auto network_end = chrono::high_resolution_clock::now();
                
                double latency_ms = chrono::duration_cast<chrono::microseconds>(network_end - network_start).count() / 1000.0;
                double throughput_mbps = (current_encrypted_data_.size() * 8.0) / (latency_ms * 1000.0);  // Mbps calculation
                
                // Log network performance
                PerformanceMetrics::NetworkMetrics net_metrics;
                net_metrics.node_type = "smart_meter";
                net_metrics.operation = "send_encrypted_data";
                net_metrics.data_size_bytes = current_encrypted_data_.size();
                net_metrics.latency_ms = latency_ms;
                net_metrics.throughput_mbps = throughput_mbps;
                net_metrics.success = success;
                net_metrics.error_type = success ? "none" : "transmission_failed";
                net_metrics.timestamp = PerformanceMetrics::getCurrentTimestamp();
                net_metrics.connection_id = meter_id_;
                PerformanceMetrics::logNetworkMetrics(net_metrics);
                
                if (success) {
                    cout << "✓ Sent " << current_encrypted_data_.size() << " bytes to " << aggregator_id << " (" << fixed << setprecision(2) << latency_ms << "ms)" << endl;
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

// Define static arrays for temporal consumption patterns
constexpr double SmartMeterContinuous::hourly_factors_[24];
constexpr double SmartMeterContinuous::daily_factors_[7];

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