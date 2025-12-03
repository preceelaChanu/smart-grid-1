#ifndef PERFORMANCE_METRICS_H
#define PERFORMANCE_METRICS_H

#include <chrono>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <iomanip>
#include <iostream>
#include <numeric>

class PerformanceMetrics {
private:
    static std::mutex csv_mutex_;
    
public:
    // Encryption performance metrics
    struct EncryptionMetrics {
        std::string algorithm;          // CKKS, BFV, BGV, AES, etc.
        size_t poly_modulus_degree;     // Security parameter
        int scale_bits;                 // CKKS scale bits
        size_t plaintext_size_bytes;    // Input data size
        size_t ciphertext_size_bytes;   // Encrypted data size
        double encryption_time_ms;      // Time to encrypt in milliseconds
        double communication_overhead;  // Network overhead
        int security_level_bits;        // Estimated security level
        std::string timestamp;
        int meter_id;                   // For tracking per-meter performance
    };
    
    // Homomorphic operation metrics
    struct HomomorphicMetrics {
        std::string operation;          // addition, multiplication, aggregation
        size_t num_operands;           // Number of ciphertexts operated on
        double operation_time_ms;      // Time to perform operation
        size_t result_size_bytes;      // Size of result
        double noise_budget_before;    // Noise budget before operation
        double noise_budget_after;     // Noise budget after operation
        std::string timestamp;
        int aggregation_cycle;         // Which aggregation cycle
    };
    
    // Network performance metrics
    struct NetworkMetrics {
        std::string node_type;         // smart_meter, aggregator, control_center
        std::string operation;         // send, receive, authenticate
        size_t data_size_bytes;        // Amount of data transferred
        double latency_ms;             // Network latency
        double throughput_mbps;        // Data throughput
        bool success;                  // Operation success/failure
        std::string error_type;        // Type of error if failed
        std::string timestamp;
        int connection_id;             // For tracking parallel connections
    };
    
    // System scalability metrics
    struct ScalabilityMetrics {
        int num_smart_meters;          // Number of active smart meters
        double total_aggregation_time_ms; // Total time for one aggregation cycle
        double memory_usage_mb;        // Memory consumption
        double cpu_usage_percent;      // CPU utilization
        int parallel_connections;      // Number of simultaneous connections
        double avg_response_time_ms;   // Average response time per meter
        double throughput_ops_per_sec; // Operations per second
        std::string timestamp;
        int cycle_number;              // Aggregation cycle number
    };
    
    // Security analysis metrics
    struct SecurityMetrics {
        std::string algorithm;
        size_t key_size_bits;
        size_t poly_modulus_degree;
        int security_level_bits;       // Estimated security level
        double security_efficiency;    // Performance per security bit
        size_t ciphertext_expansion;   // Ratio of ciphertext to plaintext size
        bool quantum_resistant;        // Quantum resistance property
        std::string attack_model;      // IND-CPA, IND-CCA2, etc.
        std::string timestamp;
    };
    
    // Static methods for logging metrics
    static void logEncryptionMetrics(const EncryptionMetrics& metrics);
    static void logHomomorphicMetrics(const HomomorphicMetrics& metrics);
    static void logNetworkMetrics(const NetworkMetrics& metrics);
    static void logScalabilityMetrics(const ScalabilityMetrics& metrics);
    static void logSecurityMetrics(const SecurityMetrics& metrics);
    
    // Utility methods
    static std::string getCurrentTimestamp();
    static void initializeCSVFiles();
    static void generateSummaryReport();
    static size_t estimateSecurityLevel(size_t poly_modulus_degree);
    static double calculateSecurityEfficiency(double performance_ms, size_t security_bits);
    
private:
    static void writeCSVHeader(const std::string& filename, const std::string& header);
    static void appendToCSV(const std::string& filename, const std::string& row);
};

// Timer utility class for automatic performance measurement
class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    std::string operation_name_;
    
public:
    PerformanceTimer(const std::string& operation) : 
        operation_name_(operation),
        start_time_(std::chrono::high_resolution_clock::now()) {}
    
    ~PerformanceTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        double duration_ms = duration.count() / 1000.0;
        
        // Log to console for debugging
        std::cout << "[PERF] " << operation_name_ << ": " << std::fixed 
                  << std::setprecision(3) << duration_ms << " ms" << std::endl;
    }
    
    double getElapsedMs() const {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time_);
        return duration.count() / 1000.0;
    }
};

// Macro for easy performance timing
#define MEASURE_PERFORMANCE(operation_name) PerformanceTimer __timer(operation_name)

#endif // PERFORMANCE_METRICS_H