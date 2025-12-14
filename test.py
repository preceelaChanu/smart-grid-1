import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

# Reload all data
files = {
    'homomorphic_metrics': '/workspaces/smart-grid-1/results/1/encryption_metrics.csv',
    'scalability_metrics': '/workspaces/smart-grid-1/results/1/homomorphic_metrics.csv',
    'security_metrics': '/workspaces/smart-grid-1/results/1/network_metrics.csv',
    'network_metrics': '/workspaces/smart-grid-1/results/1/scalability_metrics.csv',
    'encryption_metrics': '/workspaces/smart-grid-1/results/1/security_metrics.csv'
}

dataframes = {}
for name, path in files.items():
    dataframes[name] = pd.read_csv(path)

# ============================================================================
# SECTION 3: EXPERIMENTAL DATA ANALYSIS
# ============================================================================

print("\n" + "="*80)
print("2. EXPERIMENTAL IMPLEMENTATION ANALYSIS")
print("="*80)
print("\n2.1 YOUR IMPLEMENTATION OVERVIEW")
print("-" * 80)

# Analyze the implementation
impl_summary = {
    "Encryption Algorithm": "CKKS (Cheon-Kim-Kim-Song)",
    "Poly Modulus Degree": 16384,
    "Scale Bits": 50,
    "Security Level": "192 bits",
    "Key Size": "327,680 bits (40 KB)",
    "Quantum Resistant": "Yes",
    "Attack Model": "IND-CPA (Indistinguishability under Chosen Plaintext Attack)",
    "Number of Smart Meters": 500,
    "Total Data Points Collected": f"{len(dataframes['encryption_metrics'])} encryption operations",
    "Network Operations": f"{len(dataframes['network_metrics'])} network transactions",
    "Aggregation Cycles": 20
}

for key, value in impl_summary.items():
    print(f"  {key:30s}: {value}")

print("\n\n2.2 MACHINE-INDEPENDENT METRICS FROM YOUR IMPLEMENTATION")
print("-" * 80)

# Calculate machine-independent metrics
enc_df = dataframes['encryption_metrics']
sec_df = dataframes['security_metrics']
homo_df = dataframes['homomorphic_metrics']

mi_metrics = {
    "Average Plaintext Size": f"{enc_df['plaintext_size_bytes'].mean():.2f} bytes",
    "Average Ciphertext Size": f"{enc_df['ciphertext_size_bytes'].mean():.2f} bytes",
    "Communication Overhead": f"{enc_df['communication_overhead'].mean():.2f}x",
    "Ciphertext Expansion": f"{sec_df['ciphertext_expansion'].iloc[0]}x",
    "Poly Modulus Degree": f"{enc_df['poly_modulus_degree'].iloc[0]}",
    "Scale Bits": f"{enc_df['scale_bits'].iloc[0]}",
    "Security Level": f"{enc_df['security_level_bits'].iloc[0]} bits",
    "Key Size": f"{sec_df['key_size_bits'].iloc[0]} bits",
    "Number of Operands (avg)": f"{homo_df['num_operands'].mean():.0f}",
    "Noise Budget Before": f"{homo_df['noise_budget_before'].mean():.1f} bits",
    "Noise Budget After": f"{homo_df['noise_budget_after'].mean():.1f} bits",
    "Result Size": f"{homo_df['result_size_bytes'].mean():.2f} bytes"
}

print("\nMACHINE-INDEPENDENT METRICS:")
for key, value in mi_metrics.items():
    print(f"  {key:30s}: {value}")

print("\n\n2.3 MACHINE-DEPENDENT METRICS FROM YOUR IMPLEMENTATION")
print("-" * 80)

net_df = dataframes['network_metrics']
scal_df = dataframes['scalability_metrics']

md_metrics = {
    "Average Encryption Time": f"{enc_df['encryption_time_ms'].mean():.3f} ms",
    "Std Dev Encryption Time": f"{enc_df['encryption_time_ms'].std():.3f} ms",
    "Min Encryption Time": f"{enc_df['encryption_time_ms'].min():.3f} ms",
    "Max Encryption Time": f"{enc_df['encryption_time_ms'].max():.3f} ms",
    "Homomorphic Op Time (avg)": f"{homo_df['operation_time_ms'].mean():.3f} ms",
    "Total Aggregation Time (avg)": f"{scal_df['total_aggregation_time_ms'].mean():.3f} ms",
    "Memory Usage (avg)": f"{scal_df['memory_usage_mb'].mean():.2f} MB",
    "CPU Usage (avg)": f"{scal_df['cpu_usage_percent'].mean():.2f}%",
    "Network Latency (avg)": f"{net_df['latency_ms'].mean():.3f} ms",
    "Network Latency (median)": f"{net_df['latency_ms'].median():.3f} ms",
    "Throughput (avg)": f"{net_df['throughput_mbps'].mean():.2f} Mbps",
    "System Throughput": f"{scal_df['throughput_ops_per_sec'].mean():.2f} ops/sec",
    "Response Time (avg)": f"{scal_df['avg_response_time_ms'].mean():.3f} ms"
}

print("\nMACHINE-DEPENDENT METRICS (Performance Indicators):")
for key, value in md_metrics.items():
    print(f"  {key:30s}: {value}")

print("\n\n2.4 STATISTICAL DISTRIBUTION ANALYSIS")
print("-" * 80)

# Network latency analysis
net_success = net_df[net_df['success'] == True]
print(f"\nNetwork Performance:")
print(f"  Success Rate: {len(net_success)/len(net_df)*100:.2f}%")
print(f"  Failed Operations: {len(net_df[net_df['success'] == False])}")
print(f"  Latency Percentiles:")
print(f"    25th: {net_df['latency_ms'].quantile(0.25):.3f} ms")
print(f"    50th: {net_df['latency_ms'].quantile(0.50):.3f} ms")
print(f"    75th: {net_df['latency_ms'].quantile(0.75):.3f} ms")
print(f"    95th: {net_df['latency_ms'].quantile(0.95):.3f} ms")
print(f"    99th: {net_df['latency_ms'].quantile(0.99):.3f} ms")

print(f"\nEncryption Time Distribution:")
print(f"  Percentiles:")
print(f"    25th: {enc_df['encryption_time_ms'].quantile(0.25):.3f} ms")
print(f"    50th: {enc_df['encryption_time_ms'].quantile(0.50):.3f} ms")
print(f"    75th: {enc_df['encryption_time_ms'].quantile(0.75):.3f} ms")
print(f"    95th: {enc_df['encryption_time_ms'].quantile(0.95):.3f} ms")
print(f"    99th: {enc_df['encryption_time_ms'].quantile(0.99):.3f} ms")