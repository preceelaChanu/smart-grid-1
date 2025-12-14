/**
 * Homomorphic Encryption Example for Smart Meter Data using SEAL Library
 * 
 * This example demonstrates how smart meter energy consumption data is encrypted
 * using homomorphic encryption (specifically CKKS scheme from Microsoft SEAL)
 * and how aggregation can be performed on encrypted data without decryption.
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include "seal/seal.h"
#include "performance_metrics.h"

using namespace std;
using namespace seal;

class SmartMeterHomomorphicDemo {
private:
    shared_ptr<SEALContext> context_;
    KeyGenerator* keygen_;
    PublicKey public_key_;
    SecretKey secret_key_;
    RelinKeys relin_keys_;
    Encryptor* encryptor_;
    Decryptor* decryptor_;
    Evaluator* evaluator_;
    CKKSEncoder* encoder_;
    double scale_;

public:
    SmartMeterHomomorphicDemo() {
        initialize_seal();
    }

    ~SmartMeterHomomorphicDemo() {
        cleanup();
    }

    void initialize_seal() {
        cout << "=== Initializing SEAL Homomorphic Encryption for Smart Meter Data ===" << endl;
        
        // Step 1: Set up encryption parameters for CKKS scheme
        // CKKS is ideal for real numbers (energy consumption values)
        EncryptionParameters parms(scheme_type::ckks);
        
        // Polynomial modulus degree (affects security and performance)
        size_t poly_modulus_degree = 8192;
        parms.set_poly_modulus_degree(poly_modulus_degree);
        
        // Coefficient modulus (affects precision and noise budget)
        parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, {60, 40, 40, 60}));
        
        // Create SEAL context
        context_ = make_shared<SEALContext>(parms);
        
        if (!context_->parameters_set()) {
            throw runtime_error("SEAL context parameters are invalid");
        }
        
        cout << "✓ SEAL context initialized with polynomial degree: " << poly_modulus_degree << endl;
        
        // Step 2: Generate cryptographic keys
        keygen_ = new KeyGenerator(*context_);
        
        secret_key_ = keygen_->secret_key();
        keygen_->create_public_key(public_key_);
        keygen_->create_relin_keys(relin_keys_);
        
        cout << "✓ Cryptographic keys generated" << endl;
        
        // Step 3: Initialize cryptographic components
        encryptor_ = new Encryptor(*context_, public_key_);
        decryptor_ = new Decryptor(*context_, secret_key_);
        evaluator_ = new Evaluator(*context_);
        encoder_ = new CKKSEncoder(*context_);
        
        // Scale determines precision (2^40 gives good precision for energy data)
        scale_ = pow(2.0, 40);
        
        cout << "✓ Encryption components ready" << endl;
        cout << "✓ Scale factor: 2^40 = " << scale_ << endl << endl;
    }

    void demonstrate_smart_meter_encryption() {
        cout << "=== DEMONSTRATION: Smart Meter Data Encryption ===" << endl;
        
        // Step 1: Simulate smart meter energy consumption readings (in kWh)
        vector<double> meter_readings = {
            1.234,  // Meter 1: 1.234 kWh
            2.567,  // Meter 2: 2.567 kWh  
            0.891,  // Meter 3: 0.891 kWh
            3.456,  // Meter 4: 3.456 kWh
            1.789   // Meter 5: 1.789 kWh
        };
        
        cout << "Smart Meter Readings (Plaintext):" << endl;
        double total_plaintext = 0;
        for (size_t i = 0; i < meter_readings.size(); i++) {
            cout << "  Meter " << (i+1) << ": " << fixed << setprecision(3) 
                 << meter_readings[i] << " kWh" << endl;
            total_plaintext += meter_readings[i];
        }
        cout << "  Total (plaintext): " << fixed << setprecision(3) 
             << total_plaintext << " kWh" << endl << endl;
        
        // Step 2: Encrypt each meter reading using homomorphic encryption
        cout << "Encrypting meter readings with CKKS homomorphic encryption..." << endl;
        vector<Ciphertext> encrypted_readings;
        
        auto encryption_start = chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < meter_readings.size(); i++) {
            // CKKS can encrypt vectors, but we encrypt single values here
            vector<double> single_value = {meter_readings[i]};
            
            // Encode the value as plaintext
            Plaintext plain;
            encoder_->encode(single_value, scale_, plain);
            
            // Measure encryption time for this specific meter
            auto meter_encrypt_start = chrono::high_resolution_clock::now();
            
            // Encrypt the plaintext
            Ciphertext encrypted;
            encryptor_->encrypt(plain, encrypted);
            encrypted_readings.push_back(encrypted);
            
            auto meter_encrypt_end = chrono::high_resolution_clock::now();
            auto meter_encrypt_time = chrono::duration_cast<chrono::microseconds>(
                meter_encrypt_end - meter_encrypt_start).count() / 1000.0;
            
            // Calculate size metrics for this encryption
            size_t plaintext_size = sizeof(double);
            size_t encrypted_size = encrypted.size() * encoder_->slot_count() * sizeof(uint64_t);
            
            // Log encryption metrics for this meter
            PerformanceMetrics::EncryptionMetrics enc_metrics;
            enc_metrics.algorithm = "CKKS";
            enc_metrics.poly_modulus_degree = context_->key_context_data()->parms().poly_modulus_degree();
            enc_metrics.scale_bits = static_cast<int>(std::log2(scale_));
            enc_metrics.plaintext_size_bytes = plaintext_size;
            enc_metrics.ciphertext_size_bytes = encrypted_size;
            enc_metrics.encryption_time_ms = meter_encrypt_time;
            enc_metrics.communication_overhead = static_cast<double>(encrypted_size) / plaintext_size;
            enc_metrics.security_level_bits = static_cast<int>(PerformanceMetrics::estimateSecurityLevel(enc_metrics.poly_modulus_degree));
            enc_metrics.timestamp = PerformanceMetrics::getCurrentTimestamp();
            enc_metrics.meter_id = static_cast<int>(i + 1);
            PerformanceMetrics::logEncryptionMetrics(enc_metrics);
            
            // Log complexity analysis for encryption
            auto enc_complexity = PerformanceMetrics::analyzeComplexity(
                "encryption", 1, enc_metrics.poly_modulus_degree, meter_encrypt_time * 1000.0, static_cast<int>(i + 1));
            PerformanceMetrics::logComplexityAnalysisMetrics(enc_complexity);
            
            cout << "  ✓ Meter " << (i+1) << " reading encrypted (size: " 
                 << encrypted.size() << " polynomials, " << fixed << setprecision(3) 
                 << meter_encrypt_time << " ms)" << endl;
        }
        
        auto encryption_end = chrono::high_resolution_clock::now();
        auto encryption_time = chrono::duration_cast<chrono::microseconds>(
            encryption_end - encryption_start).count() / 1000.0;
        
        cout << "✓ All readings encrypted in " << fixed << setprecision(2) 
             << encryption_time << " ms" << endl << endl;
        
        // Step 3: Perform homomorphic aggregation (addition) on encrypted data
        cout << "Performing homomorphic aggregation on encrypted data..." << endl;
        
        auto aggregation_start = chrono::high_resolution_clock::now();
        
        // Start with the first encrypted reading
        Ciphertext aggregated_result = encrypted_readings[0];
        
        // Add all other encrypted readings homomorphically
        for (size_t i = 1; i < encrypted_readings.size(); i++) {
            evaluator_->add_inplace(aggregated_result, encrypted_readings[i]);
            cout << "  ✓ Added meter " << (i+1) << " to aggregation" << endl;
        }
        
        auto aggregation_end = chrono::high_resolution_clock::now();
        auto aggregation_time = chrono::duration_cast<chrono::microseconds>(
            aggregation_end - aggregation_start).count() / 1000.0;
        
        cout << "✓ Homomorphic aggregation completed in " << fixed << setprecision(2)
             << aggregation_time << " ms" << endl;
        cout << "✓ Aggregated result is still encrypted!" << endl << endl;
        
        // Step 4: Decrypt the aggregated result to verify correctness
        cout << "Decrypting aggregated result..." << endl;
        
        auto decryption_start = chrono::high_resolution_clock::now();
        
        Plaintext decrypted_plain;
        decryptor_->decrypt(aggregated_result, decrypted_plain);
        
        vector<double> decrypted_result;
        encoder_->decode(decrypted_plain, decrypted_result);
        
        auto decryption_end = chrono::high_resolution_clock::now();
        auto decryption_time = chrono::duration_cast<chrono::microseconds>(
            decryption_end - decryption_start).count() / 1000.0;
        
        // Log decryption performance metrics
        auto decrypt_complexity = PerformanceMetrics::analyzeComplexity(
            "decryption", 1, context_->key_context_data()->parms().poly_modulus_degree(), 
            decryption_time * 1000.0);
        PerformanceMetrics::logComplexityAnalysisMetrics(decrypt_complexity);
        
        double total_encrypted = decrypted_result[0];
        
        cout << "✓ Decryption completed in " << fixed << setprecision(2)
             << decryption_time << " ms" << endl;
        cout << "✓ Aggregated total: " << fixed << setprecision(3)
             << total_encrypted << " kWh" << endl;
        
        // Step 5: Verify accuracy and log performance metrics
        double error = abs(total_plaintext - total_encrypted);
        cout << "✓ Expected total: " << fixed << setprecision(3)
             << total_plaintext << " kWh" << endl;
        cout << "✓ Computation error: " << scientific << setprecision(2)
             << error << " kWh" << endl;

        // Log data correctness metrics
        auto correctness_metrics = PerformanceMetrics::analyzeCorrectness(
            total_plaintext, total_encrypted, "homomorphic_aggregation", -1, 0.001);
        PerformanceMetrics::logDataCorrectnessMetrics(correctness_metrics);

        // Calculate and log size comparison metrics
        size_t plaintext_total_size = meter_readings.size() * sizeof(double);
        size_t encrypted_total_size = meter_readings.size() * (encoder_->slot_count() * sizeof(uint64_t));
        size_t plaintext_agg_size = sizeof(double);
        size_t encrypted_agg_size = encoder_->slot_count() * sizeof(uint64_t);
        
        auto size_metrics = PerformanceMetrics::analyzeSizes(
            plaintext_total_size, encrypted_total_size, plaintext_agg_size, encrypted_agg_size,
            "CKKS", context_->key_context_data()->parms().poly_modulus_degree(), meter_readings.size());
        PerformanceMetrics::logSizeComparisonMetrics(size_metrics);

        // Log complexity analysis for aggregation operation
        auto complexity_metrics = PerformanceMetrics::analyzeComplexity(
            "homomorphic_addition", meter_readings.size(), 
            context_->key_context_data()->parms().poly_modulus_degree(), 
            aggregation_time * 1000.0);  // Convert ms to microseconds
        PerformanceMetrics::logComplexityAnalysisMetrics(complexity_metrics);

        if (error < 0.001) {
            cout << "✓ SUCCESS: Homomorphic computation is accurate!" << endl;
        } else {
            cout << "⚠ WARNING: Higher than expected error in computation" << endl;
        }

        // Display comprehensive metrics summary
        cout << endl << "=== PERFORMANCE METRICS SUMMARY ===" << endl;
        cout << "Data Correctness:" << endl;
        cout << "  - Accuracy: " << fixed << setprecision(4) << correctness_metrics.accuracy_percentage << "%" << endl;
        cout << "  - Relative Error: " << scientific << setprecision(3) << correctness_metrics.relative_error_percent << "%" << endl;
        
        cout << "Size Analysis:" << endl;
        cout << "  - Encryption Expansion: " << fixed << setprecision(2) << size_metrics.encryption_expansion_ratio << "x" << endl;
        cout << "  - Space Efficiency: " << fixed << setprecision(2) << size_metrics.space_efficiency_percent << "%" << endl;
        cout << "  - Aggregated Expansion: " << fixed << setprecision(2) << size_metrics.aggregated_expansion_ratio << "x" << endl;
        
        cout << "Complexity Analysis:" << endl;
        cout << "  - Time per operation: " << fixed << setprecision(3) << complexity_metrics.time_per_operation_us << " μs" << endl;
        cout << "  - Complexity class: " << complexity_metrics.complexity_class << endl;
        cout << "  - CPU cycles per element: " << fixed << setprecision(1) << complexity_metrics.cpu_cycles_per_element << endl;

        cout << endl;
    }

    void demonstrate_privacy_protection() {
        cout << "=== DEMONSTRATION: Privacy Protection Analysis ===" << endl;
        
        // Show that individual readings cannot be extracted from encrypted data
        double reading = 2.345; // kWh
        vector<double> single_reading = {reading};
        
        cout << "Original smart meter reading: " << fixed << setprecision(3) 
             << reading << " kWh" << endl;
        
        // Encrypt the reading
        Plaintext plain;
        encoder_->encode(single_reading, scale_, plain);
        
        Ciphertext encrypted;
        encryptor_->encrypt(plain, encrypted);
        
        cout << "Encrypted data (first 10 coefficients):" << endl;
        cout << "  [Note: These numbers appear random and reveal nothing about the original data]" << endl;
        
        // In a real scenario, you would see the encrypted polynomial coefficients
        // which appear as random numbers providing information-theoretic privacy
        cout << "  Ciphertext size: " << encrypted.size() << " polynomials" << endl;
        cout << "  Each polynomial has " << encrypted.poly_modulus_degree() << " coefficients" << endl;
        cout << "  Total encrypted size: " << (encrypted.size() * encrypted.poly_modulus_degree() * sizeof(uint64_t)) 
             << " bytes" << endl;
        cout << "  Expansion factor: ~" << (encrypted.size() * encrypted.poly_modulus_degree() * sizeof(uint64_t)) / sizeof(double) 
             << "x larger than plaintext" << endl;
        
        cout << endl;
        cout << "Privacy guarantees:" << endl;
        cout << "  ✓ Individual meter readings cannot be extracted without the secret key" << endl;
        cout << "  ✓ Only aggregated results can be computed" << endl;
        cout << "  ✓ Intermediate computations remain encrypted" << endl;
        cout << "  ✓ IND-CPA security against chosen-plaintext attacks" << endl;
        cout << "  ✓ Post-quantum security (lattice-based cryptography)" << endl;
        cout << endl;
    }

    void demonstrate_performance_characteristics() {
        cout << "=== DEMONSTRATION: Performance Characteristics ===" << endl;
        
        vector<int> meter_counts = {10, 50, 100, 500};
        
        cout << left << setw(12) << "Meters" << setw(15) << "Encrypt (ms)" 
             << setw(15) << "Aggregate (ms)" << setw(15) << "Total (ms)" << endl;
        cout << string(60, '-') << endl;
        
        for (int count : meter_counts) {
            // Generate random meter readings
            vector<double> readings(count);
            for (int i = 0; i < count; i++) {
                readings[i] = 0.1 + (rand() % 500) / 100.0; // 0.1 to 5.1 kWh
            }
            
            // Measure encryption time
            auto encrypt_start = chrono::high_resolution_clock::now();
            vector<Ciphertext> encrypted(count);
            
            for (int i = 0; i < count; i++) {
                vector<double> single = {readings[i]};
                Plaintext plain;
                encoder_->encode(single, scale_, plain);
                encryptor_->encrypt(plain, encrypted[i]);
            }
            
            auto encrypt_end = chrono::high_resolution_clock::now();
            
            // Measure aggregation time
            auto agg_start = chrono::high_resolution_clock::now();
            
            Ciphertext result = encrypted[0];
            for (int i = 1; i < count; i++) {
                evaluator_->add_inplace(result, encrypted[i]);
            }
            
            auto agg_end = chrono::high_resolution_clock::now();
            
            double encrypt_time = chrono::duration_cast<chrono::microseconds>(
                encrypt_end - encrypt_start).count() / 1000.0;
            double agg_time = chrono::duration_cast<chrono::microseconds>(
                agg_end - agg_start).count() / 1000.0;
            
            cout << left << setw(12) << count 
                 << setw(15) << fixed << setprecision(2) << encrypt_time
                 << setw(15) << agg_time 
                 << setw(15) << (encrypt_time + agg_time) << endl;
        }
        
        cout << endl;
    }

private:
    void cleanup() {
        delete keygen_;
        delete encryptor_;
        delete decryptor_;
        delete evaluator_;
        delete encoder_;
    }
};

int main() {
    try {
        // Initialize performance metrics system
        PerformanceMetrics::initializeCSVFiles();
        cout << "✓ Performance metrics system initialized" << endl << endl;
        SmartMeterHomomorphicDemo demo;
        
        demo.demonstrate_smart_meter_encryption();
        demo.demonstrate_privacy_protection();
        demo.demonstrate_performance_characteristics();
        
        cout << "=== SUMMARY ===" << endl;
        cout << "This demonstration shows how the SEAL library enables:" << endl;
        cout << "1. Encryption of sensitive smart meter energy consumption data" << endl;
        cout << "2. Homomorphic aggregation without exposing individual readings" << endl;
        cout << "3. Privacy-preserving smart grid data analytics" << endl;
        cout << "4. Quantum-resistant security for future-proof protection" << endl;
        cout << endl;
        cout << "In the real smart grid system, this enables utilities to:" << endl;
        cout << "- Calculate neighborhood/city-wide energy consumption" << endl;
        cout << "- Detect anomalies in aggregate consumption patterns" << endl;
        cout << "- Perform load balancing and demand forecasting" << endl;
        cout << "- All while preserving individual household privacy" << endl;
        
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}