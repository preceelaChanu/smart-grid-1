#ifndef KDC_CLIENT_H
#define KDC_CLIENT_H

#include "network_utils.h"
#include "seal/seal.h"
#include <memory>

using namespace std;
using namespace seal;

// Forward declarations from kdc.cpp
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
    uint8_t padding[7];
} __attribute__((packed));

struct KeyResponseMessage {
    KeyRequestStatus status;
    KeyType key_type;
    uint64_t current_version;
    uint64_t key_data_size;
    uint8_t padding[6];
} __attribute__((packed));

class KDCClient {
private:
    string kdc_host_;
    uint16_t kdc_port_;
    NodeCertificate client_cert_;
    uint64_t cached_key_version_;
    
public:
    KDCClient(const string& kdc_host, uint16_t kdc_port, const NodeCertificate& client_cert);
    
    // Request public key and relinearization keys from KDC
    bool request_public_keys(shared_ptr<SEALContext> context, PublicKey& public_key, RelinKeys& relin_keys);
    
    // Request all keys (only for control center)
    bool request_all_keys(shared_ptr<SEALContext> context, PublicKey& public_key, 
                         RelinKeys& relin_keys, SecretKey& secret_key);
    
    // Check if keys need to be updated
    bool check_key_version(uint64_t& latest_version);
    
    // Get cached version
    uint64_t get_cached_version() const { return cached_key_version_; }

private:
    bool send_key_request(KeyRequestType request_type, vector<uint8_t>& response_data);
    bool parse_key_response(const vector<uint8_t>& data, shared_ptr<SEALContext> context,
                           PublicKey* public_key, RelinKeys* relin_keys, SecretKey* secret_key = nullptr);
};

#endif // KDC_CLIENT_H