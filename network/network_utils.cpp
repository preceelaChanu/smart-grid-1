#include "network_utils.h"
#include <iostream>
#include <iomanip>
#include <cstring>
#include <errno.h>
#include <sys/select.h>
#include <openssl/evp.h>
#include <openssl/err.h>

using namespace std;

// NetworkUtils implementation
int NetworkUtils::create_server_socket(uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_network_event("ERROR", "Failed to create server socket: " + get_last_error());
        return -1;
    }
    
    // Enable address reuse
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_network_event("WARNING", "Failed to set SO_REUSEADDR: " + get_last_error());
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_network_event("ERROR", "Failed to bind server socket to port " + to_string(port) + ": " + get_last_error());
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 5) < 0) {
        log_network_event("ERROR", "Failed to listen on server socket: " + get_last_error());
        close(sockfd);
        return -1;
    }
    
    log_network_event("INFO", "Server socket created and listening on port " + to_string(port));
    return sockfd;
}

int NetworkUtils::create_client_socket(const string& host, uint16_t port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_network_event("ERROR", "Failed to create client socket: " + get_last_error());
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
        log_network_event("ERROR", "Invalid server address: " + host);
        close(sockfd);
        return -1;
    }
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_network_event("ERROR", "Failed to connect to " + host + ":" + to_string(port) + ": " + get_last_error());
        close(sockfd);
        return -1;
    }
    
    log_network_event("INFO", "Connected to server " + host + ":" + to_string(port));
    return sockfd;
}

bool NetworkUtils::set_socket_timeout(int sockfd, uint32_t timeout_sec) {
    struct timeval timeout;
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        return false;
    }
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        return false;
    }
    
    return true;
}

bool NetworkUtils::send_message(int sockfd, MessageType type, const void* payload, uint32_t payload_size) {
    NetworkMessageHeader header;
    header.msg_type = type;
    header.payload_size = payload_size;
    header.timestamp = chrono::duration_cast<chrono::seconds>(
        chrono::system_clock::now().time_since_epoch()).count();
    
    // Calculate header checksum
    header.checksum = calculate_checksum(&header, sizeof(header) - sizeof(header.checksum));
    
    // Send header
    if (send(sockfd, &header, sizeof(header), 0) != sizeof(header)) {
        log_network_event("ERROR", "Failed to send message header: " + get_last_error());
        return false;
    }
    
    // Send payload if present
    if (payload_size > 0 && payload != nullptr) {
        ssize_t sent = send(sockfd, payload, payload_size, 0);
        if (sent != payload_size) {
            log_network_event("ERROR", "Failed to send complete payload. Sent " + to_string(sent) + " of " + to_string(payload_size) + " bytes");
            return false;
        }
    }
    
    return true;
}

bool NetworkUtils::receive_message(int sockfd, NetworkMessageHeader& header, vector<uint8_t>& payload) {
    // Receive header
    ssize_t received = recv(sockfd, &header, sizeof(header), MSG_WAITALL);
    if (received != sizeof(header)) {
        if (received == 0) {
            log_network_event("INFO", "Connection closed by peer");
        } else {
            log_network_event("ERROR", "Failed to receive message header: " + get_last_error());
        }
        return false;
    }
    
    // Verify magic number and version
    if (header.magic != 0x53474944 || header.version != 1) {
        log_network_event("ERROR", "Invalid message header magic/version");
        return false;
    }
    
    // Verify payload size is reasonable
    if (header.payload_size > MAX_MESSAGE_SIZE) {
        log_network_event("ERROR", "Message payload too large: " + to_string(header.payload_size));
        return false;
    }
    
    // Receive payload
    payload.resize(header.payload_size);
    if (header.payload_size > 0) {
        received = recv(sockfd, payload.data(), header.payload_size, MSG_WAITALL);
        if (received != header.payload_size) {
            log_network_event("ERROR", "Failed to receive complete payload. Received " + to_string(received) + " of " + to_string(header.payload_size) + " bytes");
            return false;
        }
    }
    
    return verify_message_integrity(header, payload);
}

uint32_t NetworkUtils::calculate_checksum(const void* data, size_t size) {
    // Simple CRC32-like checksum
    uint32_t checksum = 0;
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    
    for (size_t i = 0; i < size; i++) {
        checksum = (checksum << 8) ^ bytes[i];
        for (int j = 0; j < 8; j++) {
            if (checksum & 0x80000000) {
                checksum = (checksum << 1) ^ 0x04C11DB7;
            } else {
                checksum = checksum << 1;
            }
        }
    }
    
    return checksum;
}

bool NetworkUtils::generate_random_bytes(uint8_t* buffer, size_t size) {
    return RAND_bytes(buffer, size) == 1;
}

bool NetworkUtils::calculate_sha256(const void* data, size_t size, uint8_t* hash) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return false;
    
    bool success = false;
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
        EVP_DigestUpdate(ctx, data, size) == 1 &&
        EVP_DigestFinal_ex(ctx, hash, nullptr) == 1) {
        success = true;
    }
    
    EVP_MD_CTX_free(ctx);
    return success;
}

bool NetworkUtils::verify_message_integrity(const NetworkMessageHeader& header, const vector<uint8_t>& payload) {
    // Verify header checksum
    NetworkMessageHeader temp_header = header;
    temp_header.checksum = 0;
    uint32_t calculated_checksum = calculate_checksum(&temp_header, sizeof(temp_header));
    
    if (calculated_checksum != header.checksum) {
        log_network_event("ERROR", "Message header checksum mismatch");
        return false;
    }
    
    return true;
}

string NetworkUtils::get_last_error() {
    return strerror(errno);
}

void NetworkUtils::log_network_event(const string& event, const string& details) {
    auto now = chrono::system_clock::now();
    auto time_t = chrono::system_clock::to_time_t(now);
    auto tm = *localtime(&time_t);
    
    cout << "[" << put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] NETWORK " << event;
    if (!details.empty()) {
        cout << ": " << details;
    }
    cout << endl;
}

// SecureConnection implementation
SecureConnection::SecureConnection(int sockfd) 
    : sockfd_(sockfd), authenticated_(false), sequence_number_(0) {
    memset(session_token_, 0, sizeof(session_token_));
    memset(&peer_certificate_, 0, sizeof(peer_certificate_));
}

SecureConnection::~SecureConnection() {
    close();
}

bool SecureConnection::authenticate_as_client(const NodeCertificate& client_cert) {
    // Send authentication request
    AuthRequestPayload auth_req;
    auth_req.certificate = client_cert;
    auth_req.client_timestamp = chrono::duration_cast<chrono::seconds>(
        chrono::system_clock::now().time_since_epoch()).count();
    
    // Generate challenge response (simplified - in production use proper crypto)
    NetworkUtils::generate_random_bytes(auth_req.challenge_response, sizeof(auth_req.challenge_response));
    
    if (!NetworkUtils::send_message(sockfd_, MessageType::AUTH_REQUEST, &auth_req, sizeof(auth_req))) {
        return false;
    }
    
    // Receive authentication response
    NetworkMessageHeader header;
    vector<uint8_t> payload;
    if (!NetworkUtils::receive_message(sockfd_, header, payload) || 
        header.msg_type != MessageType::AUTH_RESPONSE ||
        payload.size() != sizeof(AuthResponsePayload)) {
        return false;
    }
    
    AuthResponsePayload* auth_resp = reinterpret_cast<AuthResponsePayload*>(payload.data());
    if (auth_resp->status == AuthStatus::SUCCESS) {
        authenticated_ = true;
        memcpy(session_token_, auth_resp->session_token, sizeof(session_token_));
        NetworkUtils::log_network_event("INFO", "Client authentication successful");
        return true;
    } else {
        NetworkUtils::log_network_event("ERROR", "Client authentication failed: " + string(auth_resp->error_message));
        return false;
    }
}

bool SecureConnection::authenticate_as_server(const NodeCertificate& server_cert) {
    // Receive authentication request
    NetworkMessageHeader header;
    vector<uint8_t> payload;
    if (!NetworkUtils::receive_message(sockfd_, header, payload) || 
        header.msg_type != MessageType::AUTH_REQUEST ||
        payload.size() != sizeof(AuthRequestPayload)) {
        return false;
    }
    
    AuthRequestPayload* auth_req = reinterpret_cast<AuthRequestPayload*>(payload.data());
    peer_certificate_ = auth_req->certificate;
    
    // Validate client certificate
    AuthResponsePayload auth_resp;
    memset(&auth_resp, 0, sizeof(auth_resp));
    
    if (!peer_certificate_.is_valid()) {
        auth_resp.status = AuthStatus::EXPIRED_CERTIFICATE;
        strcpy(auth_resp.error_message, "Client certificate expired");
    } else if (peer_certificate_.node_type != NodeType::AGGREGATOR) {
        auth_resp.status = AuthStatus::UNKNOWN_NODE;
        strcpy(auth_resp.error_message, "Invalid node type for this service");
    } else {
        // Authentication successful
        auth_resp.status = AuthStatus::SUCCESS;
        NetworkUtils::generate_random_bytes(auth_resp.session_token, sizeof(auth_resp.session_token));
        memcpy(session_token_, auth_resp.session_token, sizeof(session_token_));
        authenticated_ = true;
        strcpy(auth_resp.error_message, "Authentication successful");
    }
    
    auth_resp.server_timestamp = chrono::duration_cast<chrono::seconds>(
        chrono::system_clock::now().time_since_epoch()).count();
    
    bool sent = NetworkUtils::send_message(sockfd_, MessageType::AUTH_RESPONSE, &auth_resp, sizeof(auth_resp));
    
    if (authenticated_) {
        NetworkUtils::log_network_event("INFO", "Server authentication successful for node: " + string(peer_certificate_.node_id));
    } else {
        NetworkUtils::log_network_event("WARNING", "Server authentication failed: " + string(auth_resp.error_message));
    }
    
    return sent && authenticated_;
}

bool SecureConnection::send_secure_data(const void* data, size_t size) {
    if (!authenticated_) {
        NetworkUtils::log_network_event("ERROR", "Cannot send data on unauthenticated connection");
        return false;
    }
    
    DataTransferHeader dt_header;
    memcpy(dt_header.session_token, session_token_, sizeof(session_token_));
    dt_header.data_size = size;
    dt_header.chunk_number = 1;
    dt_header.total_chunks = 1;
    
    // Calculate data hash
    NetworkUtils::calculate_sha256(data, size, dt_header.data_hash);
    
    // Send data transfer header + data
    vector<uint8_t> payload(sizeof(dt_header) + size);
    memcpy(payload.data(), &dt_header, sizeof(dt_header));
    memcpy(payload.data() + sizeof(dt_header), data, size);
    
    return NetworkUtils::send_message(sockfd_, MessageType::DATA_TRANSFER, payload.data(), payload.size());
}

bool SecureConnection::receive_secure_data(vector<uint8_t>& data) {
    if (!authenticated_) {
        NetworkUtils::log_network_event("ERROR", "Cannot receive data on unauthenticated connection");
        return false;
    }
    
    NetworkMessageHeader header;
    vector<uint8_t> payload;
    if (!NetworkUtils::receive_message(sockfd_, header, payload) || 
        header.msg_type != MessageType::DATA_TRANSFER ||
        payload.size() < sizeof(DataTransferHeader)) {
        return false;
    }
    
    DataTransferHeader* dt_header = reinterpret_cast<DataTransferHeader*>(payload.data());
    
    // Verify session token
    if (memcmp(dt_header->session_token, session_token_, sizeof(session_token_)) != 0) {
        NetworkUtils::log_network_event("ERROR", "Invalid session token in data transfer");
        return false;
    }
    
    // Extract data
    size_t data_offset = sizeof(DataTransferHeader);
    size_t actual_data_size = payload.size() - data_offset;
    
    if (actual_data_size != dt_header->data_size) {
        NetworkUtils::log_network_event("ERROR", "Data size mismatch in transfer");
        return false;
    }
    
    data.resize(actual_data_size);
    memcpy(data.data(), payload.data() + data_offset, actual_data_size);
    
    // Verify data hash
    uint8_t calculated_hash[32];
    NetworkUtils::calculate_sha256(data.data(), data.size(), calculated_hash);
    if (memcmp(calculated_hash, dt_header->data_hash, 32) != 0) {
        NetworkUtils::log_network_event("ERROR", "Data integrity check failed");
        return false;
    }
    
    // Send acknowledgment
    NetworkUtils::send_message(sockfd_, MessageType::DATA_ACK, nullptr, 0);
    
    return true;
}

void SecureConnection::close() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
    authenticated_ = false;
}

bool SecureConnection::authenticate_as_kdc_server() {
    // Receive authentication request
    NetworkMessageHeader header;
    vector<uint8_t> payload;
    if (!NetworkUtils::receive_message(sockfd_, header, payload) || 
        header.msg_type != MessageType::AUTH_REQUEST ||
        payload.size() != sizeof(AuthRequestPayload)) {
        return false;
    }
    
    AuthRequestPayload* auth_req = reinterpret_cast<AuthRequestPayload*>(payload.data());
    peer_certificate_ = auth_req->certificate;
    
    // Validate client certificate - KDC accepts all valid node types
    AuthResponsePayload auth_resp;
    memset(&auth_resp, 0, sizeof(auth_resp));
    
    if (!peer_certificate_.is_valid()) {
        auth_resp.status = AuthStatus::EXPIRED_CERTIFICATE;
        strcpy(auth_resp.error_message, "Client certificate expired");
    } else if (peer_certificate_.node_type != NodeType::SMART_METER && 
               peer_certificate_.node_type != NodeType::AGGREGATOR && 
               peer_certificate_.node_type != NodeType::CONTROL_CENTER) {
        auth_resp.status = AuthStatus::UNKNOWN_NODE;
        strcpy(auth_resp.error_message, "Invalid node type for KDC service");
    } else {
        // Authentication successful
        auth_resp.status = AuthStatus::SUCCESS;
        NetworkUtils::generate_random_bytes(auth_resp.session_token, sizeof(auth_resp.session_token));
        memcpy(session_token_, auth_resp.session_token, sizeof(session_token_));
        authenticated_ = true;
        strcpy(auth_resp.error_message, "Authentication successful");
    }
    
    auth_resp.server_timestamp = chrono::duration_cast<chrono::seconds>(
        chrono::system_clock::now().time_since_epoch()).count();
    
    bool sent = NetworkUtils::send_message(sockfd_, MessageType::AUTH_RESPONSE, &auth_resp, sizeof(auth_resp));
    
    if (authenticated_) {
        NetworkUtils::log_network_event("INFO", "KDC authentication successful for node: " + string(peer_certificate_.node_id));
    } else {
        NetworkUtils::log_network_event("ERROR", "KDC authentication failed: " + string(auth_resp.error_message));
    }
    
    return authenticated_ && sent;
}

bool NetworkUtils::load_certificate(const string& cert_file, NodeCertificate& cert) {
    FILE* file = fopen(cert_file.c_str(), "rb");
    if (!file) return false;
    
    bool success = fread(&cert, sizeof(cert), 1, file) == 1;
    fclose(file);
    return success;
}

bool NetworkUtils::save_certificate(const string& cert_file, const NodeCertificate& cert) {
    FILE* file = fopen(cert_file.c_str(), "wb");
    if (!file) return false;
    
    bool success = fwrite(&cert, sizeof(cert), 1, file) == 1;
    fclose(file);
    return success;
}