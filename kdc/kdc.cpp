#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <fstream>
#include <signal.h>
#include "seal/seal.h"
#include "json.hpp"
#include "network_utils.h"

using namespace std;
using namespace seal;
using json = nlohmann::json;

// Protocol message structures
enum class KeyRequestType : uint8_t {
    PUBLIC_KEY_ONLY = 0x01,
    ALL_KEYS = 0x02,
    VERSION_CHECK = 0x03
};

enum class KeyRequestStatus : uint8_t {
    SUCCESS = 0x00,
    UNAUTHORIZED = 0x01,
    INVALID_REQUEST = 0x02,
    KEY_GENERATION_FAILED = 0x03
};

enum class KeyType : uint8_t {
    PUBLIC_KEY = 0x01,
    ALL_KEYS = 0x02
};

struct KeyRequestMessage {
    KeyRequestType request_type;
    uint64_t requested_version;
    uint8_t padding[7];  // Alignment
} __attribute__((packed));

struct KeyResponseMessage {
    KeyRequestStatus status;
    KeyType key_type;
    uint64_t current_version;
    uint64_t key_data_size;
    uint8_t padding[6];  // Alignment
} __attribute__((packed));

class KeyDistributionCenter {
private:
    int server_sockfd_;
    bool running_;
    json config_;
    NodeCertificate kdc_cert_;
    
    // Key storage
    PublicKey public_key_;
    RelinKeys relin_keys_;
    SEALContext context_;
    
    // Session management
    mutex sessions_mutex_;
    unordered_map<string, chrono::time_point<chrono::steady_clock>> active_sessions_;
    
    // Key version management
    uint64_t current_key_version_;
    mutex key_version_mutex_;
    
public:
    KeyDistributionCenter() : running_(false), current_key_version_(1), context_(EncryptionParameters(scheme_type::ckks)) {}
    
    bool initialize() {
        cout << "=== Key Distribution Center (KDC) Server ===" << endl;
        cout << "Initializing secure key distribution service..." << endl;
        
        // Load configuration
        ifstream config_file("config.json");
        if (!config_file.is_open()) {
            cerr << "Error: Could not open config.json" << endl;
            return false;
        }
        config_file >> config_;
        config_file.close();
        
        // Load KDC certificate
        string kdc_cert_file = config_["key_distribution_center"]["certificate"];
        if (!NetworkUtils::load_certificate(kdc_cert_file, kdc_cert_)) {
            cerr << "Error: Could not load KDC certificate: " << kdc_cert_file << endl;
            return false;
        }
        
        cout << "Loaded KDC certificate: " << kdc_cert_.node_id << endl;
        
        // Initialize SEAL context with current parameters
        size_t poly_modulus_degree = config_["poly_modulus_degree"];
        EncryptionParameters parms(scheme_type::ckks);
        parms.set_poly_modulus_degree(poly_modulus_degree);
        parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
        
        context_ = SEALContext(parms);
        if (!context_.parameters_set()) {
            cerr << "Error: SEAL context parameters are invalid" << endl;
            return false;
        }
        
        cout << "SEAL context initialized" << endl;
        
        // Load existing keys
        if (!load_keys()) {
            cout << "No existing keys found, will generate on first request" << endl;
        } else {
            cout << "Loaded existing cryptographic keys (version " << current_key_version_ << ")" << endl;
        }
        
        // Create server socket
        uint16_t port = config_["key_distribution_center"]["port"];
        server_sockfd_ = NetworkUtils::create_server_socket(port);
        if (server_sockfd_ < 0) {
            cerr << "Error: Failed to create KDC server socket on port " << port << endl;
            return false;
        }
        
        cout << "✓ KDC server listening on port " << port << endl;
        cout << "Ready to serve cryptographic keys to authorized nodes" << endl;
        
        return true;
    }
    
    void run() {
        running_ = true;
        cout << "KDC server started - waiting for key requests..." << endl;
        
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int client_sockfd = accept(server_sockfd_, (struct sockaddr*)&client_addr, &client_len);
            if (client_sockfd < 0) {
                if (running_) {
                    NetworkUtils::log_network_event("KDC_ACCEPT_ERROR", NetworkUtils::get_last_error());
                }
                continue;
            }
            
            // Handle client in separate thread
            thread client_thread(&KeyDistributionCenter::handle_client, this, client_sockfd, client_addr);
            client_thread.detach();
        }
    }
    
    void stop() {
        running_ = false;
        if (server_sockfd_ >= 0) {
            close(server_sockfd_);
        }
        cout << "KDC server stopped" << endl;
    }

private:
    bool load_keys() {
        try {
            // Load public key
            ifstream pk_file(config_["public_key_file"], ios::binary);
            if (!pk_file.is_open()) return false;
            public_key_.load(context_, pk_file);
            pk_file.close();
            
            // Load relinearization keys
            ifstream rk_file(config_["relin_keys_file"], ios::binary);
            if (!rk_file.is_open()) return false;
            relin_keys_.load(context_, rk_file);
            rk_file.close();
            
            return true;
        } catch (const exception& e) {
            cerr << "Error loading keys: " << e.what() << endl;
            return false;
        }
    }
    
    bool generate_fresh_keys() {
        cout << "Generating fresh cryptographic keys..." << endl;
        
        KeyGenerator keygen(context_);
        
        // Generate new keys
        PublicKey new_public_key;
        keygen.create_public_key(new_public_key);
        
        RelinKeys new_relin_keys;
        keygen.create_relin_keys(new_relin_keys);
        
        // Save keys
        ofstream pk_file(config_["public_key_file"], ios::binary);
        if (!pk_file.is_open()) return false;
        new_public_key.save(pk_file);
        pk_file.close();
        
        ofstream rk_file(config_["relin_keys_file"], ios::binary);
        if (!rk_file.is_open()) return false;
        new_relin_keys.save(rk_file);
        rk_file.close();
        
        // Update in-memory keys
        public_key_ = new_public_key;
        relin_keys_ = new_relin_keys;
        
        // Increment version
        lock_guard<mutex> lock(key_version_mutex_);
        current_key_version_++;
        
        cout << "Generated fresh keys (version " << current_key_version_ << ")" << endl;
        return true;
    }
    
    void handle_client(int client_sockfd, struct sockaddr_in client_addr) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        cout << "✓ Accepted connection from: " << client_ip << ":" << ntohs(client_addr.sin_port) << endl;
        
        SecureConnection secure_conn(client_sockfd);
        
        // Use KDC-specific authentication (accepts all node types)
        if (!secure_conn.authenticate_as_kdc_server()) {
            cerr << "Error: Client authentication failed from " << client_ip << endl;
            return;
        }
        
        string client_node_id = secure_conn.get_peer_certificate().node_id;
        NodeType client_type = secure_conn.get_peer_certificate().node_type;
        
        cout << "✓ Authenticated client: " << client_node_id 
             << " (type: " << static_cast<int>(client_type) << ")" << endl;
        
        // Process key request
        handle_key_request(secure_conn, client_node_id, client_type);
        
        cout << "Session completed with " << client_node_id << endl;
    }
    
    void handle_key_request(SecureConnection& conn, const string& client_id, NodeType client_type) {
        // Receive key request
        vector<uint8_t> request_data;
        if (!conn.receive_secure_data(request_data)) {
            cerr << "Error: Failed to receive key request from " << client_id << endl;
            return;
        }
        
        if (request_data.size() < sizeof(KeyRequestMessage)) {
            cerr << "Error: Invalid key request size from " << client_id << endl;
            return;
        }
        
        KeyRequestMessage* request = reinterpret_cast<KeyRequestMessage*>(request_data.data());
        
        cout << "Key request from " << client_id << ":" << endl;
        cout << "  Requested version: " << request->requested_version << endl;
        cout << "  Request type: " << static_cast<int>(request->request_type) << endl;
        
        // Determine what keys to send based on node type
        KeyResponseMessage response;
        response.current_version = current_key_version_;
        response.status = KeyRequestStatus::SUCCESS;
        
        vector<uint8_t> key_data;
        
        if (request->request_type == KeyRequestType::PUBLIC_KEY_ONLY || 
            client_type == NodeType::SMART_METER || 
            client_type == NodeType::AGGREGATOR) {
            // Send public key only (smart meters and aggregator don't need secret key)
            response.key_type = KeyType::PUBLIC_KEY;
            key_data = serialize_public_key();
            cout << "  Sending public key only (appropriate for node type)" << endl;
            
        } else if (request->request_type == KeyRequestType::ALL_KEYS && 
                   client_type == NodeType::CONTROL_CENTER) {
            // Send all keys (only control center gets secret key)
            response.key_type = KeyType::ALL_KEYS;
            key_data = serialize_all_keys();
            cout << "  Sending complete key set (control center authorized)" << endl;
            
        } else {
            response.status = KeyRequestStatus::UNAUTHORIZED;
            cout << "  Request denied - insufficient privileges" << endl;
        }
        
        // Send response header
        if (!conn.send_secure_data(&response, sizeof(response))) {
            cerr << "Error: Failed to send key response header to " << client_id << endl;
            return;
        }
        
        // Send key data if authorized
        if (response.status == KeyRequestStatus::SUCCESS && !key_data.empty()) {
            if (!conn.send_secure_data(key_data.data(), key_data.size())) {
                cerr << "Error: Failed to send key data to " << client_id << endl;
                return;
            }
        }
        
        cout << "✓ Key distribution completed for " << client_id << endl;
        
        // Update session tracking
        lock_guard<mutex> lock(sessions_mutex_);
        active_sessions_[client_id] = chrono::steady_clock::now();
    }
    
    vector<uint8_t> serialize_public_key() {
        stringstream stream;
        public_key_.save(stream);
        relin_keys_.save(stream);
        
        string serialized = stream.str();
        return vector<uint8_t>(serialized.begin(), serialized.end());
    }
    
    vector<uint8_t> serialize_all_keys() {
        stringstream stream;
        public_key_.save(stream);
        relin_keys_.save(stream);
        
        // Note: In production, secret key should be handled more securely
        // This is for demonstration purposes only
        ifstream sk_file(config_["secret_key_file"], ios::binary);
        if (sk_file.is_open()) {
            stream << sk_file.rdbuf();
            sk_file.close();
        }
        
        string serialized = stream.str();
        return vector<uint8_t>(serialized.begin(), serialized.end());
    }

public:
    // Key rotation API
    bool rotate_keys() {
        cout << "Initiating key rotation..." << endl;
        
        if (!generate_fresh_keys()) {
            cerr << "Error: Failed to generate new keys during rotation" << endl;
            return false;
        }
        
        cout << "✓ Key rotation completed - new version: " << current_key_version_ << endl;
        cout << "Note: Clients should request updated keys" << endl;
        
        return true;
    }
    
    void print_status() {
        lock_guard<mutex> lock(sessions_mutex_);
        cout << "\n=== KDC Status Report ===" << endl;
        cout << "Current key version: " << current_key_version_ << endl;
        cout << "Active sessions: " << active_sessions_.size() << endl;
        
        auto now = chrono::steady_clock::now();
        for (const auto& session : active_sessions_) {
            auto elapsed = chrono::duration_cast<chrono::seconds>(now - session.second);
            cout << "  " << session.first << " (last seen: " << elapsed.count() << "s ago)" << endl;
        }
        cout << "=========================" << endl;
    }
};

// Global KDC instance
KeyDistributionCenter* kdc_instance = nullptr;

void signal_handler(int signal) {
    cout << "\nReceived shutdown signal (" << signal << ")" << endl;
    if (kdc_instance) {
        kdc_instance->stop();
    }
}

int main() {
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    KeyDistributionCenter kdc;
    kdc_instance = &kdc;
    
    if (!kdc.initialize()) {
        cerr << "Error: Failed to initialize KDC" << endl;
        return 1;
    }
    
    auto start_time = chrono::high_resolution_clock::now();
    
    cout << "\n=== KDC Operational ===" << endl;
    cout << "Secure key distribution service is now active" << endl;
    cout << "Authorized nodes can request cryptographic keys" << endl;
    cout << "Press Ctrl+C to shutdown gracefully" << endl;
    
    // Run main server loop
    kdc.run();
    
    auto end_time = chrono::high_resolution_clock::now();
    auto runtime = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    
    cout << "\n=== KDC Service Complete ===" << endl;
    cout << "Total runtime: " << runtime.count() << " seconds" << endl;
    cout << "Key distribution service terminated" << endl;
    
    return 0;
}