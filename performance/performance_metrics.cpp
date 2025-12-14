#include "performance_metrics.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <cmath>

std::mutex PerformanceMetrics::csv_mutex_;

std::string PerformanceMetrics::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

void PerformanceMetrics::initializeCSVFiles() {
    std::lock_guard<std::mutex> lock(csv_mutex_);
    
    // Create performance_data directory if it doesn't exist
    std::filesystem::create_directories("performance_data");
    
    // Initialize CSV files with headers
    writeCSVHeader("performance_data/encryption_metrics.csv", 
        "timestamp,algorithm,poly_modulus_degree,scale_bits,plaintext_size_bytes,"
        "ciphertext_size_bytes,encryption_time_ms,"
        "communication_overhead,security_level_bits,meter_id");
    
    writeCSVHeader("performance_data/homomorphic_metrics.csv",
        "timestamp,operation,num_operands,operation_time_ms,result_size_bytes,"
        "noise_budget_before,noise_budget_after,aggregation_cycle");
    
    writeCSVHeader("performance_data/network_metrics.csv",
        "timestamp,node_type,operation,data_size_bytes,latency_ms,throughput_mbps,"
        "success,error_type,connection_id");
    
    writeCSVHeader("performance_data/scalability_metrics.csv",
        "timestamp,num_smart_meters,total_aggregation_time_ms,memory_usage_mb,"
        "cpu_usage_percent,parallel_connections,avg_response_time_ms,"
        "throughput_ops_per_sec,cycle_number");
    
    writeCSVHeader("performance_data/security_metrics.csv",
        "timestamp,algorithm,key_size_bits,poly_modulus_degree,security_level_bits,"
        "security_efficiency,ciphertext_expansion,quantum_resistant,attack_model");
        
    writeCSVHeader("performance_data/data_correctness_metrics.csv",
        "timestamp,expected_value,actual_value,absolute_error,relative_error_percent,"
        "accuracy_percentage,is_correct,threshold,operation_type,num_operations,meter_id");
        
    writeCSVHeader("performance_data/size_comparison_metrics.csv",
        "timestamp,plaintext_size_bytes,encrypted_size_bytes,encryption_expansion_ratio,"
        "plaintext_aggregated_size_bytes,encrypted_aggregated_size_bytes,aggregated_expansion_ratio,"
        "space_efficiency_percent,algorithm,poly_modulus_degree,num_values_aggregated");
        
    writeCSVHeader("performance_data/complexity_analysis_metrics.csv",
        "timestamp,operation_type,input_size,poly_modulus_degree,time_per_operation_us,"
        "time_complexity_factor,memory_operations,arithmetic_operations,cpu_cycles_per_element,"
        "theoretical_complexity,complexity_class,iteration_number");
    
    std::cout << "✓ Performance metrics CSV files initialized in performance_data/" << std::endl;
}

void PerformanceMetrics::logEncryptionMetrics(const EncryptionMetrics& metrics) {
    std::lock_guard<std::mutex> lock(csv_mutex_);
    
    std::stringstream row;
    row << metrics.timestamp << ","
        << metrics.algorithm << ","
        << metrics.poly_modulus_degree << ","
        << metrics.scale_bits << ","
        << metrics.plaintext_size_bytes << ","
        << metrics.ciphertext_size_bytes << ","
        << std::fixed << std::setprecision(3) << metrics.encryption_time_ms << ","
        << std::fixed << std::setprecision(3) << metrics.communication_overhead << ","
        << metrics.security_level_bits << ","
        << metrics.meter_id;
    
    appendToCSV("performance_data/encryption_metrics.csv", row.str());
}

void PerformanceMetrics::logHomomorphicMetrics(const HomomorphicMetrics& metrics) {
    std::lock_guard<std::mutex> lock(csv_mutex_);
    
    std::stringstream row;
    row << metrics.timestamp << ","
        << metrics.operation << ","
        << metrics.num_operands << ","
        << std::fixed << std::setprecision(3) << metrics.operation_time_ms << ","
        << metrics.result_size_bytes << ","
        << std::fixed << std::setprecision(3) << metrics.noise_budget_before << ","
        << std::fixed << std::setprecision(3) << metrics.noise_budget_after << ","
        << metrics.aggregation_cycle;
    
    appendToCSV("performance_data/homomorphic_metrics.csv", row.str());
}

void PerformanceMetrics::logNetworkMetrics(const NetworkMetrics& metrics) {
    std::lock_guard<std::mutex> lock(csv_mutex_);
    
    std::stringstream row;
    row << metrics.timestamp << ","
        << metrics.node_type << ","
        << metrics.operation << ","
        << metrics.data_size_bytes << ","
        << std::fixed << std::setprecision(3) << metrics.latency_ms << ","
        << std::fixed << std::setprecision(3) << metrics.throughput_mbps << ","
        << (metrics.success ? "true" : "false") << ","
        << metrics.error_type << ","
        << metrics.connection_id;
    
    appendToCSV("performance_data/network_metrics.csv", row.str());
}

void PerformanceMetrics::logScalabilityMetrics(const ScalabilityMetrics& metrics) {
    std::lock_guard<std::mutex> lock(csv_mutex_);
    
    std::stringstream row;
    row << metrics.timestamp << ","
        << metrics.num_smart_meters << ","
        << std::fixed << std::setprecision(3) << metrics.total_aggregation_time_ms << ","
        << std::fixed << std::setprecision(3) << metrics.memory_usage_mb << ","
        << std::fixed << std::setprecision(3) << metrics.cpu_usage_percent << ","
        << metrics.parallel_connections << ","
        << std::fixed << std::setprecision(3) << metrics.avg_response_time_ms << ","
        << std::fixed << std::setprecision(3) << metrics.throughput_ops_per_sec << ","
        << metrics.cycle_number;
    
    appendToCSV("performance_data/scalability_metrics.csv", row.str());
}

void PerformanceMetrics::logSecurityMetrics(const SecurityMetrics& metrics) {
    std::lock_guard<std::mutex> lock(csv_mutex_);
    
    std::stringstream row;
    row << metrics.timestamp << ","
        << metrics.algorithm << ","
        << metrics.key_size_bits << ","
        << metrics.poly_modulus_degree << ","
        << metrics.security_level_bits << ","
        << std::fixed << std::setprecision(6) << metrics.security_efficiency << ","
        << metrics.ciphertext_expansion << ","
        << (metrics.quantum_resistant ? "true" : "false") << ","
        << metrics.attack_model;
    
    appendToCSV("performance_data/security_metrics.csv", row.str());
}

void PerformanceMetrics::logDataCorrectnessMetrics(const DataCorrectnessMetrics& metrics) {
    std::lock_guard<std::mutex> lock(csv_mutex_);
    
    std::stringstream row;
    row << metrics.timestamp << ","
        << std::fixed << std::setprecision(9) << metrics.expected_value << ","
        << std::fixed << std::setprecision(9) << metrics.actual_value << ","
        << std::scientific << std::setprecision(6) << metrics.absolute_error << ","
        << std::fixed << std::setprecision(6) << metrics.relative_error_percent << ","
        << std::fixed << std::setprecision(4) << metrics.accuracy_percentage << ","
        << (metrics.is_correct ? "true" : "false") << ","
        << std::scientific << std::setprecision(3) << metrics.threshold << ","
        << metrics.operation_type << ","
        << metrics.num_operations << ","
        << metrics.meter_id;
    
    appendToCSV("performance_data/data_correctness_metrics.csv", row.str());
}

void PerformanceMetrics::logSizeComparisonMetrics(const SizeComparisonMetrics& metrics) {
    std::lock_guard<std::mutex> lock(csv_mutex_);
    
    std::stringstream row;
    row << metrics.timestamp << ","
        << metrics.plaintext_size_bytes << ","
        << metrics.encrypted_size_bytes << ","
        << std::fixed << std::setprecision(4) << metrics.encryption_expansion_ratio << ","
        << metrics.plaintext_aggregated_size_bytes << ","
        << metrics.encrypted_aggregated_size_bytes << ","
        << std::fixed << std::setprecision(4) << metrics.aggregated_expansion_ratio << ","
        << std::fixed << std::setprecision(4) << metrics.space_efficiency_percent << ","
        << metrics.algorithm << ","
        << metrics.poly_modulus_degree << ","
        << metrics.num_values_aggregated;
    
    appendToCSV("performance_data/size_comparison_metrics.csv", row.str());
}

void PerformanceMetrics::logComplexityAnalysisMetrics(const ComplexityAnalysisMetrics& metrics) {
    std::lock_guard<std::mutex> lock(csv_mutex_);
    
    std::stringstream row;
    row << metrics.timestamp << ","
        << metrics.operation_type << ","
        << metrics.input_size << ","
        << metrics.poly_modulus_degree << ","
        << std::fixed << std::setprecision(6) << metrics.time_per_operation_us << ","
        << std::fixed << std::setprecision(9) << metrics.time_complexity_factor << ","
        << metrics.memory_operations << ","
        << metrics.arithmetic_operations << ","
        << std::fixed << std::setprecision(4) << metrics.cpu_cycles_per_element << ","
        << std::fixed << std::setprecision(6) << metrics.theoretical_complexity << ","
        << metrics.complexity_class << ","
        << metrics.iteration_number;
    
    appendToCSV("performance_data/complexity_analysis_metrics.csv", row.str());
}

void PerformanceMetrics::writeCSVHeader(const std::string& filename, const std::string& header) {
    std::ofstream file(filename, std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file << header << std::endl;
        file.close();
    }
}

void PerformanceMetrics::appendToCSV(const std::string& filename, const std::string& row) {
    std::ofstream file(filename, std::ios::out | std::ios::app);
    if (file.is_open()) {
        file << row << std::endl;
        file.close();
    }
}

size_t PerformanceMetrics::estimateSecurityLevel(size_t poly_modulus_degree) {
    // Security level estimation based on lattice cryptography standards
    // These are approximate values for CKKS/BFV schemes
    if (poly_modulus_degree >= 32768) return 256;      // Paranoid
    if (poly_modulus_degree >= 16384) return 192;      // Very High
    if (poly_modulus_degree >= 8192) return 128;       // High
    if (poly_modulus_degree >= 4096) return 96;        // Medium
    return 80;  // Low
}

double PerformanceMetrics::calculateSecurityEfficiency(double performance_ms, size_t security_bits) {
    // Security efficiency = Security bits per millisecond
    if (performance_ms <= 0) return 0.0;
    return static_cast<double>(security_bits) / performance_ms;
}

void PerformanceMetrics::generateSummaryReport() {
    std::lock_guard<std::mutex> lock(csv_mutex_);
    
    std::ofstream report("performance_data/summary_report.txt");
    if (!report.is_open()) return;
    
    report << "=== SMART GRID PERFORMANCE ANALYSIS SUMMARY ===" << std::endl;
    report << "Generated at: " << getCurrentTimestamp() << std::endl;
    report << std::endl;
    
    report << "Available Data Files:" << std::endl;
    report << "- encryption_metrics.csv: Encryption/decryption performance per meter" << std::endl;
    report << "- homomorphic_metrics.csv: Homomorphic operation performance" << std::endl;
    report << "- network_metrics.csv: Network communication performance" << std::endl;
    report << "- scalability_metrics.csv: System scalability with varying smart meter counts" << std::endl;
    report << "- security_metrics.csv: Security parameter analysis" << std::endl;
    report << std::endl;
    
    report << "Research Comparison Metrics Available:" << std::endl;
    report << "1. Algorithm Performance: CKKS vs BFV vs BGV vs AES" << std::endl;
    report << "2. Security vs Performance Trade-offs" << std::endl;
    report << "3. Scalability Analysis: Overhead with increasing smart meters" << std::endl;
    report << "4. Network Overhead: Communication costs" << std::endl;
    report << "5. Homomorphic Operation Efficiency" << std::endl;
    report << std::endl;
    
    report << "Suggested Analysis:" << std::endl;
    report << "- Plot encryption time vs security level for different algorithms" << std::endl;
    report << "- Analyze aggregation time scaling with number of smart meters" << std::endl;
    report << "- Compare ciphertext expansion ratios across schemes" << std::endl;
    report << "- Measure network throughput degradation with encryption" << std::endl;
    
    report.close();
    
    std::cout << "✓ Performance summary report generated: performance_data/summary_report.txt" << std::endl;
}

PerformanceMetrics::DataCorrectnessMetrics PerformanceMetrics::analyzeCorrectness(
    double expected, double actual, const std::string& operation_type, 
    int meter_id, double threshold) {
    
    DataCorrectnessMetrics metrics;
    metrics.expected_value = expected;
    metrics.actual_value = actual;
    metrics.absolute_error = std::abs(expected - actual);
    
    if (std::abs(expected) > 1e-15) {
        metrics.relative_error_percent = (metrics.absolute_error / std::abs(expected)) * 100.0;
    } else {
        metrics.relative_error_percent = 0.0;
    }
    
    metrics.accuracy_percentage = 100.0 - metrics.relative_error_percent;
    metrics.is_correct = (metrics.absolute_error <= threshold);
    metrics.threshold = threshold;
    metrics.operation_type = operation_type;
    metrics.num_operations = 1;
    metrics.timestamp = getCurrentTimestamp();
    metrics.meter_id = meter_id;
    
    return metrics;
}

PerformanceMetrics::SizeComparisonMetrics PerformanceMetrics::analyzeSizes(
    size_t plaintext_size, size_t encrypted_size, 
    size_t plaintext_agg_size, size_t encrypted_agg_size,
    const std::string& algorithm, size_t poly_degree, int num_values) {
    
    SizeComparisonMetrics metrics;
    metrics.plaintext_size_bytes = plaintext_size;
    metrics.encrypted_size_bytes = encrypted_size;
    metrics.encryption_expansion_ratio = (plaintext_size > 0) ? 
        static_cast<double>(encrypted_size) / plaintext_size : 0.0;
    
    metrics.plaintext_aggregated_size_bytes = plaintext_agg_size;
    metrics.encrypted_aggregated_size_bytes = encrypted_agg_size;
    metrics.aggregated_expansion_ratio = (plaintext_agg_size > 0) ? 
        static_cast<double>(encrypted_agg_size) / plaintext_agg_size : 0.0;
    
    metrics.space_efficiency_percent = (metrics.encryption_expansion_ratio > 0) ?
        100.0 / metrics.encryption_expansion_ratio : 0.0;
    
    metrics.algorithm = algorithm;
    metrics.poly_modulus_degree = poly_degree;
    metrics.num_values_aggregated = num_values;
    metrics.timestamp = getCurrentTimestamp();
    
    return metrics;
}

PerformanceMetrics::ComplexityAnalysisMetrics PerformanceMetrics::analyzeComplexity(
    const std::string& operation_type, size_t input_size, size_t poly_degree,
    double execution_time_us, int iteration) {
    
    ComplexityAnalysisMetrics metrics;
    metrics.operation_type = operation_type;
    metrics.input_size = input_size;
    metrics.poly_modulus_degree = poly_degree;
    metrics.time_per_operation_us = execution_time_us;
    
    // Calculate normalized time complexity factor
    // For homomorphic operations, complexity is typically O(n log n) where n = poly_degree
    if (poly_degree > 0) {
        double log_n = std::log2(static_cast<double>(poly_degree));
        metrics.time_complexity_factor = execution_time_us / (poly_degree * log_n);
        
        // Estimate theoretical complexity class
        if (operation_type == "encryption" || operation_type == "decryption") {
            metrics.theoretical_complexity = poly_degree * log_n;
            metrics.complexity_class = "O(n log n)";
        } else if (operation_type == "addition") {
            metrics.theoretical_complexity = poly_degree;
            metrics.complexity_class = "O(n)";
        } else if (operation_type == "multiplication" || operation_type == "relinearization") {
            metrics.theoretical_complexity = poly_degree * log_n * log_n;
            metrics.complexity_class = "O(n log²n)";
        } else {
            metrics.theoretical_complexity = poly_degree;
            metrics.complexity_class = "O(n)";
        }
    } else {
        metrics.time_complexity_factor = 0.0;
        metrics.theoretical_complexity = 0.0;
        metrics.complexity_class = "O(1)";
    }
    
    // Estimate memory and arithmetic operations
    if (operation_type == "encryption" || operation_type == "decryption") {
        metrics.memory_operations = poly_degree * 2;  // Read input, write output
        metrics.arithmetic_operations = poly_degree * std::log2(poly_degree) * 4;  // NTT operations
    } else if (operation_type == "addition") {
        metrics.memory_operations = poly_degree * 3;  // Read two operands, write result
        metrics.arithmetic_operations = poly_degree;  // Simple element-wise addition
    } else if (operation_type == "multiplication") {
        metrics.memory_operations = poly_degree * 3;  // Read two operands, write result
        metrics.arithmetic_operations = poly_degree * std::log2(poly_degree) * 6;  // Complex multiplication
    } else {
        metrics.memory_operations = poly_degree;
        metrics.arithmetic_operations = poly_degree;
    }
    
    // Estimate CPU cycles per element (very rough approximation)
    metrics.cpu_cycles_per_element = (input_size > 0) ? 
        (execution_time_us * 2.5) / input_size : 0.0;  // Assuming 2.5 GHz CPU
    
    metrics.iteration_number = iteration;
    metrics.timestamp = getCurrentTimestamp();
    
    return metrics;
}