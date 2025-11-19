#include "kdc_client.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

using json = nlohmann::json;

KDCClient::KDCClient(const string& kdc_host, uint16_t kdc_port, const NodeCertificate& client_cert)
    : kdc_host_(kdc_host), kdc_port_(kdc_port), client_cert_(client_cert), cached_key_version_(0) {
}

bool KDCClient::request_public_keys(shared_ptr<SEALContext> context, PublicKey& public_key, RelinKeys& relin_keys) {
    cout << "Requesting public keys from KDC..." << endl;
    
    vector<uint8_t> response_data;
    if (!send_key_request(KeyRequestType::PUBLIC_KEY_ONLY, response_data)) {
        cerr << "Error: Failed to send key request to KDC" << endl;
        return false;
    }
    
    if (!parse_key_response(response_data, context, &public_key, &relin_keys)) {
        cerr << "Error: Failed to parse key response from KDC" << endl;
        return false;
    }
    
    cout << "✓ Successfully received public keys from KDC (version " << cached_key_version_ << ")" << endl;
    return true;
}

bool KDCClient::request_all_keys(shared_ptr<SEALContext> context, PublicKey& public_key, 
                                 RelinKeys& relin_keys, SecretKey& secret_key) {
    cout << "Requesting all keys from KDC..." << endl;
    
    vector<uint8_t> response_data;
    if (!send_key_request(KeyRequestType::ALL_KEYS, response_data)) {
        cerr << "Error: Failed to send key request to KDC" << endl;
        return false;
    }
    
    if (!parse_key_response(response_data, context, &public_key, &relin_keys, &secret_key)) {
        cerr << "Error: Failed to parse key response from KDC" << endl;
        return false;
    }
    
    cout << "✓ Successfully received all keys from KDC (version " << cached_key_version_ << ")" << endl;
    return true;
}

bool KDCClient::check_key_version(uint64_t& latest_version) {
    vector<uint8_t> response_data;
    if (!send_key_request(KeyRequestType::VERSION_CHECK, response_data)) {
        return false;
    }
    
    if (response_data.size() < sizeof(KeyResponseMessage)) {
        return false;
    }
    
    KeyResponseMessage* response = reinterpret_cast<KeyResponseMessage*>(response_data.data());
    latest_version = response->current_version;
    
    return response->status == KeyRequestStatus::SUCCESS;
}

bool KDCClient::send_key_request(KeyRequestType request_type, vector<uint8_t>& response_data) {
    // Connect to KDC
    int sockfd = NetworkUtils::create_client_socket(kdc_host_, kdc_port_);
    if (sockfd < 0) {
        cerr << "Error: Failed to connect to KDC at " << kdc_host_ << ":" << kdc_port_ << endl;
        return false;
    }
    
    SecureConnection secure_conn(sockfd);
    
    // Authenticate with KDC
    if (!secure_conn.authenticate_as_client(client_cert_)) {
        cerr << "Error: Failed to authenticate with KDC" << endl;
        return false;
    }
    
    cout << "✓ Authenticated with KDC server" << endl;
    
    // Send key request
    KeyRequestMessage request;
    request.request_type = request_type;
    request.requested_version = cached_key_version_;
    
    if (!secure_conn.send_secure_data(&request, sizeof(request))) {
        cerr << "Error: Failed to send key request" << endl;
        return false;
    }
    
    // Receive response
    if (!secure_conn.receive_secure_data(response_data)) {
        cerr << "Error: Failed to receive key response" << endl;
        return false;
    }
    
    return true;
}

bool KDCClient::parse_key_response(const vector<uint8_t>& data, shared_ptr<SEALContext> context,
                                  PublicKey* public_key, RelinKeys* relin_keys, SecretKey* secret_key) {
    if (data.size() < sizeof(KeyResponseMessage)) {
        cerr << "Error: Invalid key response size" << endl;
        return false;
    }
    
    const KeyResponseMessage* response = reinterpret_cast<const KeyResponseMessage*>(data.data());
    
    if (response->status != KeyRequestStatus::SUCCESS) {
        cerr << "Error: KDC returned status: " << static_cast<int>(response->status) << endl;
        return false;
    }
    
    cached_key_version_ = response->current_version;
    
    // Parse key data
    const uint8_t* key_data = data.data() + sizeof(KeyResponseMessage);
    size_t key_data_size = data.size() - sizeof(KeyResponseMessage);
    
    try {
        stringstream key_stream(string(reinterpret_cast<const char*>(key_data), key_data_size));
        
        // Load public key
        if (public_key) {
            public_key->load(*context, key_stream);
        }
        
        // Load relinearization keys
        if (relin_keys) {
            relin_keys->load(*context, key_stream);
        }
        
        // Load secret key if requested and available
        if (secret_key && response->key_type == KeyType::ALL_KEYS) {
            secret_key->load(*context, key_stream);
        }
        
        return true;
        
    } catch (const exception& e) {
        cerr << "Error parsing keys from KDC response: " << e.what() << endl;
        return false;
    }
}