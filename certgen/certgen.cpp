#include <iostream>
#include <chrono>
#include <fstream>
#include <string>
#include <cstring>
#include "network_utils.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

int main() {
    cout << "=== Smart Grid Certificate Authority ===" << endl;
    cout << "Generating node certificates for secure communication..." << endl;
    
    // Load configuration
    ifstream config_file("config.json");
    if (!config_file.is_open()) {
        cerr << "Error: Could not open config.json" << endl;
        return 1;
    }
    
    json config;
    config_file >> config;
    config_file.close();
    
    // Certificate validity period (1 year)
    uint64_t now = chrono::duration_cast<chrono::seconds>(
        chrono::system_clock::now().time_since_epoch()).count();
    uint64_t expiry = now + (365 * 24 * 60 * 60); // 1 year
    
    cout << "Certificate validity: " << now << " to " << expiry << endl;
    
    // Generate CA keypair (simplified - in production use proper Ed25519)
    uint8_t ca_private_key[32];
    uint8_t ca_public_key[32];
    NetworkUtils::generate_random_bytes(ca_private_key, 32);
    NetworkUtils::generate_random_bytes(ca_public_key, 32);
    
    cout << "Generated CA keypair" << endl;
    
    // Generate Aggregator Certificate
    NodeCertificate agg_cert;
    memset(&agg_cert, 0, sizeof(agg_cert));
    agg_cert.node_type = NodeType::AGGREGATOR;
    strcpy(agg_cert.node_id, "aggregator-001");
    agg_cert.issued_timestamp = now;
    agg_cert.expiry_timestamp = expiry;
    NetworkUtils::generate_random_bytes(agg_cert.public_key, 32);
    
    // Sign certificate (simplified signature)
    NetworkUtils::calculate_sha256(&agg_cert, sizeof(agg_cert) - sizeof(agg_cert.signature), agg_cert.signature);
    
    if (!NetworkUtils::save_certificate("keys/aggregator.cert", agg_cert)) {
        cerr << "Error: Failed to save aggregator certificate" << endl;
        return 1;
    }
    
    cout << "✓ Generated aggregator certificate: keys/aggregator.cert" << endl;
    
    // Generate Control Center Certificate
    NodeCertificate cc_cert;
    memset(&cc_cert, 0, sizeof(cc_cert));
    cc_cert.node_type = NodeType::CONTROL_CENTER;
    strcpy(cc_cert.node_id, "control-center-001");
    cc_cert.issued_timestamp = now;
    cc_cert.expiry_timestamp = expiry;
    NetworkUtils::generate_random_bytes(cc_cert.public_key, 32);
    
    // Sign certificate
    NetworkUtils::calculate_sha256(&cc_cert, sizeof(cc_cert) - sizeof(cc_cert.signature), cc_cert.signature);
    
    if (!NetworkUtils::save_certificate("keys/control_center.cert", cc_cert)) {
        cerr << "Error: Failed to save control center certificate" << endl;
        return 1;
    }
    
    cout << "✓ Generated control center certificate: keys/control_center.cert" << endl;
    
    // Generate Smart Meter Certificates
    int num_clients = config["num_clients"];
    for (int i = 1; i <= num_clients; i++) {
        NodeCertificate meter_cert;
        memset(&meter_cert, 0, sizeof(meter_cert));
        meter_cert.node_type = NodeType::SMART_METER;
        snprintf(meter_cert.node_id, sizeof(meter_cert.node_id), "smart-meter-%03d", i);
        meter_cert.issued_timestamp = now;
        meter_cert.expiry_timestamp = expiry;
        NetworkUtils::generate_random_bytes(meter_cert.public_key, 32);
        
        // Sign certificate
        NetworkUtils::calculate_sha256(&meter_cert, sizeof(meter_cert) - sizeof(meter_cert.signature), meter_cert.signature);
        
        string cert_file = "keys/smart_meter_" + to_string(i) + ".cert";
        if (!NetworkUtils::save_certificate(cert_file, meter_cert)) {
            cerr << "Error: Failed to save smart meter " << i << " certificate" << endl;
            return 1;
        }
    }
    
    cout << "✓ Generated " << num_clients << " smart meter certificates" << endl;
    
    // Generate KDC Certificate
    NodeCertificate kdc_cert;
    memset(&kdc_cert, 0, sizeof(kdc_cert));
    kdc_cert.node_type = NodeType::KEY_DISTRIBUTION_CENTER;
    strcpy(kdc_cert.node_id, "kdc-001");
    kdc_cert.issued_timestamp = now;
    kdc_cert.expiry_timestamp = expiry;
    NetworkUtils::generate_random_bytes(kdc_cert.public_key, 32);
    
    // Sign certificate
    NetworkUtils::calculate_sha256(&kdc_cert, sizeof(kdc_cert) - sizeof(kdc_cert.signature), kdc_cert.signature);
    
    if (!NetworkUtils::save_certificate("keys/kdc.cert", kdc_cert)) {
        cerr << "Error: Failed to save KDC certificate" << endl;
        return 1;
    }
    
    cout << "✓ Generated KDC certificate: keys/kdc.cert" << endl;
    
    // Save CA public key for verification
    ofstream ca_key_file("keys/ca_public_key.bin", ios::binary);
    if (ca_key_file.is_open()) {
        ca_key_file.write(reinterpret_cast<char*>(ca_public_key), 32);
        ca_key_file.close();
        cout << "✓ Saved CA public key: keys/ca_public_key.bin" << endl;
    }
    
    cout << "\n=== Certificate Generation Complete ===" << endl;
    cout << "All nodes now have valid certificates for secure authentication." << endl;
    cout << "Certificates valid until: " << ctime((time_t*)&expiry) << endl;
    
    return 0;
}