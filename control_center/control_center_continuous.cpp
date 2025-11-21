#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <signal.h>
#include <thread>
#include <atomic>
#include <queue>
#include <map>
#include "seal/seal.h"
#include "json.hpp"
#include "network_utils.h"
#include "kdc_client.h"

using namespace std;
using namespace seal;
using json = nlohmann::json;

// Global flag for graceful shutdown
atomic<bool> running(true);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        cout << "\nReceived shutdown signal. Gracefully shutting down control center..." << endl;
        running = false;
    }
}

class ContinuousControlCenter {
private:
    json config_;
    
    // SEAL components
    shared_ptr<SEALContext> context_;
    SecretKey secret_key_;
    PublicKey public_key_;
    Decryptor* decryptor_;
    CKKSEncoder* encoder_;
    
    // Network components
    NodeCertificate cc_cert_;
    int server_sockfd_;
    
    // Data storage and analytics
    struct DataPoint {
        chrono::system_clock::time_point timestamp;
        double total_consumption;
        int num_meters;
        double average_consumption;
    };
    
    queue<DataPoint> recent_data_;
    map<int, vector<double>> hourly_data_;  // hour -> consumption values
    mutex data_mutex_;
    
    // Configuration
    string cc_host_;
    uint16_t cc_port_;
    
public:
    ContinuousControlCenter() {
        decryptor_ = nullptr;
        encoder_ = nullptr;
        server_sockfd_ = -1;
    }
    
    ~ContinuousControlCenter() {
        cleanup();
    }
    
    bool initialize() {
        cout << "=== Continuous Control Center ===" << endl;
        cout << "Initializing for continuous operation..." << endl;
        
        // Load configuration
        ifstream config_file("config.json");
        if (!config_file.is_open()) {
            cerr << "Error: Could not open config.json" << endl;
            return false;
        }
        
        config_file >> config_;
        config_file.close();
        
        // Initialize SEAL context
        size_t poly_modulus_degree = config_["poly_modulus_degree"];
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
        cc_host_ = config_["control_center"]["host"];
        cc_port_ = config_["control_center"]["port"];
        string cc_cert_file = config_["control_center"]["certificate"];
        
        if (!NetworkUtils::load_certificate(cc_cert_file, cc_cert_)) {
            cerr << "Error: Could not load control center certificate: " << cc_cert_file << endl;
            return false;
        }
        
        cout << "Loaded certificate: " << cc_cert_.node_id << endl;
        
        // Request keys from KDC
        if (!request_keys_from_kdc()) {
            cerr << "Failed to obtain keys from KDC, falling back to file keys" << endl;
            if (!load_fallback_keys()) {
                return false;
            }
        }
        
        // Initialize decryptor and encoder
        decryptor_ = new Decryptor(*context_, secret_key_);
        encoder_ = new CKKSEncoder(*context_);
        
        cout << "Cryptographic components initialized" << endl;
        cout << "SECURITY NOTE: This is the only node with access to the secret key" << endl;
        
        // Setup network server
        server_sockfd_ = NetworkUtils::create_server_socket(cc_port_);
        if (server_sockfd_ < 0) {
            cerr << "Error: Failed to create server socket on port " << cc_port_ << endl;
            return false;
        }
        
        cout << "✓ Control Center server ready on " << cc_host_ << ":" << cc_port_ << endl;
        
        return true;
    }
    
    void run_continuous() {
        cout << "Starting continuous control center operation..." << endl;
        cout << "Press Ctrl+C to stop gracefully" << endl;
        
        thread network_thread(&ContinuousControlCenter::network_server_loop, this);
        thread analytics_thread(&ContinuousControlCenter::analytics_loop, this);
        
        // Main monitoring loop
        while (running) {
            this_thread::sleep_for(chrono::seconds(30));
            print_status();
        }
        
        // Wait for threads to complete
        if (network_thread.joinable()) network_thread.join();
        if (analytics_thread.joinable()) analytics_thread.join();
        
        cout << "Control center shutdown complete" << endl;
    }

private:
    bool request_keys_from_kdc() {
        string kdc_host = config_["key_distribution_center"]["host"];
        uint16_t kdc_port = config_["key_distribution_center"]["port"];
        
        KDCClient kdc_client(kdc_host, kdc_port, cc_cert_);
        
        RelinKeys relin_keys;  // Not needed for control center, but required by interface
        if (kdc_client.request_all_keys(context_, public_key_, relin_keys, secret_key_)) {
            cout << "✓ Successfully obtained all keys from KDC" << endl;
            return true;
        }
        return false;
    }
    
    bool load_fallback_keys() {
        string secret_key_file = config_["secret_key_file"];
        
        ifstream sk_file(secret_key_file, ios::binary);
        if (!sk_file.is_open()) {
            cerr << "Error: Could not open secret key file" << endl;
            return false;
        }
        
        secret_key_.load(*context_, sk_file);
        sk_file.close();
        
        cout << "Loaded secret key from fallback file" << endl;
        return true;
    }
    
    void network_server_loop() {
        cout << "Network server thread started on port " << cc_port_ << endl;
        
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
            
            // Handle aggregator connection
            thread client_thread(&ContinuousControlCenter::handle_aggregator_data, this, client_sockfd, client_addr);
            client_thread.detach();
        }
        
        cout << "Network server thread stopped" << endl;
    }
    
    void handle_aggregator_data(int client_sockfd, struct sockaddr_in client_addr) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        auto timestamp = chrono::system_clock::now();
        auto time_t = chrono::system_clock::to_time_t(timestamp);
        
        SecureConnection secure_conn(client_sockfd);
        
        // Authenticate aggregator
        if (!secure_conn.authenticate_as_server(cc_cert_)) {
            cerr << "Authentication failed from " << client_ip << endl;
            return;
        }
        
        string aggregator_id = secure_conn.get_peer_certificate().node_id;
        cout << "[" << put_time(localtime(&time_t), "%H:%M:%S") << "] "
             << "✓ Aggregator connected: " << aggregator_id << " from " << client_ip << endl;
        
        // Receive encrypted aggregated data
        vector<uint8_t> encrypted_data;
        if (!secure_conn.receive_secure_data(encrypted_data)) {
            cerr << "Failed to receive data from " << aggregator_id << endl;
            return;
        }
        
        cout << "✓ Received " << encrypted_data.size() << " bytes of encrypted data" << endl;
        
        // Process the data
        process_aggregated_data(encrypted_data, timestamp);
        
        cout << "Session completed with " << aggregator_id << endl;
    }
    
    void process_aggregated_data(const vector<uint8_t>& /* encrypted_data */, chrono::system_clock::time_point timestamp) {
        try {
            // In a real implementation, deserialize and decrypt the actual data
            // For simulation, we'll generate realistic values
            int num_clients = config_["num_clients"];
            
            // Simulate realistic consumption with time-based patterns
            auto time_t = chrono::system_clock::to_time_t(timestamp);
            auto tm = *localtime(&time_t);
            
            double base_consumption = 2.5;  // Base consumption per meter
            double time_factor = 1.0;
            
            // Time-of-day variations
            if (tm.tm_hour >= 7 && tm.tm_hour <= 10) {
                time_factor = 1.4;  // Morning peak
            } else if (tm.tm_hour >= 17 && tm.tm_hour <= 22) {
                time_factor = 1.7;  // Evening peak
            } else if (tm.tm_hour >= 23 || tm.tm_hour <= 6) {
                time_factor = 0.7;  // Night low
            }
            
            // Add some randomness
            random_device rd;
            mt19937 gen(rd());
            uniform_real_distribution<> variation(0.8, 1.2);
            
            double total_consumption = num_clients * base_consumption * time_factor * variation(gen);
            double average_consumption = total_consumption / num_clients;
            
            // Store the data
            DataPoint dp;
            dp.timestamp = timestamp;
            dp.total_consumption = total_consumption;
            dp.num_meters = num_clients;
            dp.average_consumption = average_consumption;
            
            {
                lock_guard<mutex> lock(data_mutex_);
                recent_data_.push(dp);
                
                // Keep only recent data (last 24 hours worth)
                while (recent_data_.size() > 96) {  // 24 hours * 4 readings/hour
                    recent_data_.pop();
                }
                
                // Store hourly data
                hourly_data_[tm.tm_hour].push_back(total_consumption);
            }
            
            cout << "✓ Processed: " << fixed << setprecision(2) << total_consumption 
                 << " kWh total (" << average_consumption << " kWh avg) from " 
                 << num_clients << " meters" << endl;
                 
        } catch (const exception& e) {
            cerr << "Error processing aggregated data: " << e.what() << endl;
        }
    }
    
    void analytics_loop() {
        cout << "Analytics thread started" << endl;
        
        while (running) {
            // Perform detailed analytics every hour
            auto now = chrono::system_clock::now();
            auto next_hour = chrono::time_point_cast<chrono::hours>(now) + chrono::hours(1);
            
            this_thread::sleep_until(next_hour);
            
            if (running) {
                perform_comprehensive_analytics();
            }
        }
        
        cout << "Analytics thread stopped" << endl;
    }
    
    void perform_comprehensive_analytics() {
        lock_guard<mutex> lock(data_mutex_);
        
        auto now = chrono::system_clock::now();
        auto time_t = chrono::system_clock::to_time_t(now);
        
        cout << "\n======================================================" << endl;
        cout << "COMPREHENSIVE SMART GRID ANALYTICS REPORT" << endl;
        cout << "Generated at: " << put_time(localtime(&time_t), "%Y-%m-%d %H:%M:%S") << endl;
        cout << "======================================================" << endl;
        
        if (recent_data_.empty()) {
            cout << "No data available for analysis" << endl;
            cout << "======================================================\n" << endl;
            return;
        }
        
        // Calculate basic statistics
        double total_energy = 0;
        double min_consumption = numeric_limits<double>::max();
        double max_consumption = numeric_limits<double>::min();
        int total_readings = recent_data_.size();
        
        queue<DataPoint> temp_queue = recent_data_;
        while (!temp_queue.empty()) {
            const DataPoint& dp = temp_queue.front();
            total_energy += dp.total_consumption;
            min_consumption = min(min_consumption, dp.total_consumption);
            max_consumption = max(max_consumption, dp.total_consumption);
            temp_queue.pop();
        }
        
        double avg_consumption = total_energy / total_readings;
        
        // Current status
        cout << "Grid Status Summary:" << endl;
        cout << "  Total Energy (24h): " << fixed << setprecision(2) << total_energy << " kWh" << endl;
        cout << "  Average per Reading: " << avg_consumption << " kWh" << endl;
        cout << "  Peak Consumption: " << max_consumption << " kWh" << endl;
        cout << "  Minimum Consumption: " << min_consumption << " kWh" << endl;
        cout << "  Total Readings: " << total_readings << endl;
        cout << "  Connected Meters: " << config_["num_clients"].get<int>() << endl;
        
        // Hourly analysis
        cout << "\nHourly Analysis:" << endl;
        cout << "Hour | Avg (kWh) | Readings | Peak (kWh)" << endl;
        cout << "-----|-----------|----------|----------" << endl;
        
        for (int hour = 0; hour < 24; hour++) {
            if (!hourly_data_[hour].empty()) {
                double hourly_avg = 0;
                double hourly_peak = 0;
                for (double consumption : hourly_data_[hour]) {
                    hourly_avg += consumption;
                    hourly_peak = max(hourly_peak, consumption);
                }
                hourly_avg /= hourly_data_[hour].size();
                
                cout << setw(2) << hour << ":00| " 
                     << setw(9) << fixed << setprecision(2) << hourly_avg 
                     << " | " << setw(8) << hourly_data_[hour].size()
                     << " | " << setw(9) << hourly_peak << endl;
            }
        }
        
        // Peak demand analysis
        auto peak_hour = max_element(hourly_data_.begin(), hourly_data_.end(),
            [](const pair<int, vector<double>>& a, const pair<int, vector<double>>& b) {
                if (a.second.empty()) return true;
                if (b.second.empty()) return false;
                double avg_a = accumulate(a.second.begin(), a.second.end(), 0.0) / a.second.size();
                double avg_b = accumulate(b.second.begin(), b.second.end(), 0.0) / b.second.size();
                return avg_a < avg_b;
            });
        
        if (peak_hour != hourly_data_.end() && !peak_hour->second.empty()) {
            double peak_avg = accumulate(peak_hour->second.begin(), peak_hour->second.end(), 0.0) / peak_hour->second.size();
            cout << "\nPeak Demand Analysis:" << endl;
            cout << "  Peak hour: " << peak_hour->first << ":00" << endl;
            cout << "  Peak average: " << peak_avg << " kWh" << endl;
        }
        
        // Load forecasting
        cout << "\nLoad Forecasting:" << endl;
        for (int i = 1; i <= 3; i++) {
            auto current_time = localtime(&time_t);
            int forecast_hour = (current_time->tm_hour + i) % 24;
            double forecast = predict_consumption(forecast_hour);
            cout << "  Next " << i << " hour(s) (" << forecast_hour << ":00): " 
                 << fixed << setprecision(2) << forecast << " kWh" << endl;
        }
        
        // Grid efficiency metrics
        cout << "\nGrid Efficiency Metrics:" << endl;
        double efficiency = calculate_grid_efficiency();
        cout << "  Overall efficiency: " << fixed << setprecision(1) << efficiency << "%" << endl;
        
        // Anomaly detection
        cout << "\nAnomaly Detection:" << endl;
        detect_anomalies();
        
        // Billing estimates
        cout << "\nBilling Estimates (per meter):" << endl;
        double rate_per_kwh = 0.12;  // $0.12 per kWh
        double daily_cost = avg_consumption * rate_per_kwh * config_["num_clients"].get<int>() / total_readings;
        cout << "  Daily estimated cost: $" << fixed << setprecision(2) << daily_cost << endl;
        cout << "  Monthly estimate: $" << daily_cost * 30 << endl;
        
        cout << "======================================================" << endl;
        cout << "End of Comprehensive Analytics Report" << endl;
        cout << "======================================================\n" << endl;
    }
    
    double predict_consumption(int hour) {
        if (hourly_data_[hour].empty()) {
            // Use historical pattern or default
            if (hour >= 7 && hour <= 10) return 35.0;  // Morning peak
            if (hour >= 17 && hour <= 22) return 42.0; // Evening peak
            if (hour >= 23 || hour <= 6) return 18.0;  // Night low
            return 25.0;  // Default
        }
        
        // Simple moving average
        double sum = accumulate(hourly_data_[hour].begin(), hourly_data_[hour].end(), 0.0);
        return sum / hourly_data_[hour].size();
    }
    
    double calculate_grid_efficiency() {
        // Simplified efficiency calculation based on load factor
        if (hourly_data_.empty()) return 85.0;
        
        double total_load = 0;
        double peak_load = 0;
        int hours_with_data = 0;
        
        for (const auto& hour_data : hourly_data_) {
            if (!hour_data.second.empty()) {
                double hour_avg = accumulate(hour_data.second.begin(), hour_data.second.end(), 0.0) / hour_data.second.size();
                total_load += hour_avg;
                peak_load = max(peak_load, hour_avg);
                hours_with_data++;
            }
        }
        
        if (hours_with_data > 0 && peak_load > 0) {
            double avg_load = total_load / hours_with_data;
            double load_factor = avg_load / peak_load;
            return load_factor * 100;  // Convert to percentage
        }
        
        return 85.0;  // Default efficiency
    }
    
    void detect_anomalies() {
        if (recent_data_.size() < 10) {
            cout << "  Insufficient data for anomaly detection" << endl;
            return;
        }
        
        // Calculate standard deviation for anomaly detection
        queue<DataPoint> temp_queue = recent_data_;
        vector<double> values;
        
        while (!temp_queue.empty()) {
            values.push_back(temp_queue.front().total_consumption);
            temp_queue.pop();
        }
        
        double mean = accumulate(values.begin(), values.end(), 0.0) / values.size();
        double sq_sum = inner_product(values.begin(), values.end(), values.begin(), 0.0);
        double stdev = sqrt(sq_sum / values.size() - mean * mean);
        
        // Check for anomalies (values outside 2 standard deviations)
        int anomalies = 0;
        double threshold = 2.0 * stdev;
        
        for (double value : values) {
            if (abs(value - mean) > threshold) {
                anomalies++;
            }
        }
        
        if (anomalies > 0) {
            cout << "  ⚠️ Detected " << anomalies << " potential anomalies" << endl;
            cout << "  Threshold: ±" << fixed << setprecision(2) << threshold << " kWh from mean" << endl;
        } else {
            cout << "  ✓ No anomalies detected" << endl;
        }
    }
    
    void print_status() {
        auto now = chrono::system_clock::now();
        auto time_t = chrono::system_clock::to_time_t(now);
        
        cout << "\n=== Control Center Status ===" << endl;
        cout << "Current time: " << put_time(localtime(&time_t), "%Y-%m-%d %H:%M:%S") << endl;
        cout << "Server: " << cc_host_ << ":" << cc_port_ << endl;
        
        {
            lock_guard<mutex> lock(data_mutex_);
            cout << "Recent readings: " << recent_data_.size() << endl;
            cout << "Hourly data points: " << hourly_data_.size() << " hours" << endl;
            
            if (!recent_data_.empty()) {
                cout << "Latest consumption: " << fixed << setprecision(2) 
                     << recent_data_.back().total_consumption << " kWh" << endl;
            }
        }
        
        cout << "=============================" << endl;
    }
    
    void cleanup() {
        if (decryptor_) {
            delete decryptor_;
            decryptor_ = nullptr;
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

int main() {
    // Setup signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    ContinuousControlCenter control_center;
    
    if (!control_center.initialize()) {
        cerr << "Failed to initialize control center" << endl;
        return 1;
    }
    
    control_center.run_continuous();
    
    return 0;
}