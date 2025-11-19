#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

using namespace std;

// Network protocol constants
const uint16_t DEFAULT_CC_PORT = 8443;
const uint32_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024; // 10MB max message
const uint32_t HANDSHAKE_TIMEOUT_SEC = 30;
const uint32_t SOCKET_TIMEOUT_SEC = 60;

// Message types
enum class MessageType : uint8_t {
    HANDSHAKE_REQUEST = 0x01,
    HANDSHAKE_RESPONSE = 0x02,
    AUTH_REQUEST = 0x03,
    AUTH_RESPONSE = 0x04,
    DATA_TRANSFER = 0x05,
    DATA_ACK = 0x06,
    ERROR_MSG = 0xFF
};

// Authentication status codes
enum class AuthStatus : uint8_t {
    SUCCESS = 0x00,
    INVALID_CERTIFICATE = 0x01,
    INVALID_SIGNATURE = 0x02,
    EXPIRED_CERTIFICATE = 0x03,
    UNKNOWN_NODE = 0x04,
    NETWORK_ERROR = 0xFF
};

// Node types
enum class NodeType : uint8_t {
    AGGREGATOR = 0x01,
    CONTROL_CENTER = 0x02,
    SMART_METER = 0x03,
    KEY_DISTRIBUTION_CENTER = 0x04
};

// Network message header
struct NetworkMessageHeader {
    uint32_t magic;           // Protocol magic number: 0x53474944 ("SGID")
    uint8_t version;          // Protocol version
    MessageType msg_type;     // Message type
    uint16_t flags;           // Message flags
    uint32_t payload_size;    // Size of payload following header
    uint64_t timestamp;       // Message timestamp
    uint32_t sequence_number; // Message sequence number
    uint32_t checksum;        // Header checksum
    
    NetworkMessageHeader() :
        magic(0x53474944), version(1), msg_type(MessageType::HANDSHAKE_REQUEST),
        flags(0), payload_size(0), timestamp(0), sequence_number(0), checksum(0) {}
} __attribute__((packed));

// Node certificate structure
struct NodeCertificate {
    NodeType node_type;
    char node_id[64];
    uint64_t issued_timestamp;
    uint64_t expiry_timestamp;
    uint8_t public_key[32];    // Ed25519 public key
    uint8_t signature[64];     // Certificate signature
    
    bool is_valid() const {
        uint64_t now = chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count();
        return now >= issued_timestamp && now <= expiry_timestamp;
    }
} __attribute__((packed));

// Authentication request payload
struct AuthRequestPayload {
    NodeCertificate certificate;
    uint8_t challenge_response[32];  // Response to server challenge
    uint64_t client_timestamp;
} __attribute__((packed));

// Authentication response payload
struct AuthResponsePayload {
    AuthStatus status;
    uint8_t session_token[32];       // Session token for subsequent messages
    uint64_t server_timestamp;
    char error_message[256];
} __attribute__((packed));

// Data transfer payload header
struct DataTransferHeader {
    uint8_t session_token[32];
    uint64_t data_size;
    uint32_t chunk_number;
    uint32_t total_chunks;
    uint8_t data_hash[32];           // SHA-256 hash of data
} __attribute__((packed));

class NetworkUtils {
public:
    // Socket utilities
    static int create_server_socket(uint16_t port);
    static int create_client_socket(const string& host, uint16_t port);
    static bool set_socket_timeout(int sockfd, uint32_t timeout_sec);
    static bool set_socket_nonblocking(int sockfd, bool nonblocking = true);
    
    // Message handling
    static bool send_message(int sockfd, MessageType type, const void* payload, uint32_t payload_size);
    static bool receive_message(int sockfd, NetworkMessageHeader& header, vector<uint8_t>& payload);
    static uint32_t calculate_checksum(const void* data, size_t size);
    
    // Cryptographic utilities
    static bool generate_random_bytes(uint8_t* buffer, size_t size);
    static bool calculate_sha256(const void* data, size_t size, uint8_t* hash);
    static bool verify_message_integrity(const NetworkMessageHeader& header, const vector<uint8_t>& payload);
    
    // Certificate management
    static bool load_certificate(const string& cert_file, NodeCertificate& cert);
    static bool save_certificate(const string& cert_file, const NodeCertificate& cert);
    static bool verify_certificate_signature(const NodeCertificate& cert, const uint8_t* ca_public_key);
    
    // Network error handling
    static string get_last_error();
    static void log_network_event(const string& event, const string& details = "");
};

class SecureConnection {
private:
    int sockfd_;
    bool authenticated_;
    uint8_t session_token_[32];
    uint32_t sequence_number_;
    NodeCertificate peer_certificate_;
    
public:
    SecureConnection(int sockfd);
    ~SecureConnection();
    
    // Authentication
    bool authenticate_as_client(const NodeCertificate& client_cert);
    bool authenticate_as_server(const NodeCertificate& server_cert);
    
    // Secure data transfer
    bool send_secure_data(const void* data, size_t size);
    bool receive_secure_data(vector<uint8_t>& data);
    
    // Connection status
    bool is_authenticated() const { return authenticated_; }
    const NodeCertificate& get_peer_certificate() const { return peer_certificate_; }
    
    // Connection management
    void close();
};

#endif // NETWORK_UTILS_H