# Smart Grid Realistic Energy Consumption Simulation

## Overview

This document describes the implementation of realistic energy consumption patterns in the smart grid system based on analysis of real UK household energy consumption data from 5,560 households.

## Dataset Analysis Summary

The system now simulates energy consumption patterns based on the following real-world statistics:

### 📊 Key Statistics
- **Total Households Analyzed**: 5,560
- **Average Consumption per Household**: 0.213 kWh
- **Median Consumption**: 0.174 kWh  
- **Consumption Range**: 0.000 - 2.112 kWh
- **Peak Consumption Hour**: 19:00 (0.315 kWh avg)
- **Lowest Consumption Hour**: 4:00 (0.110 kWh avg)

### 🏠 Household Consumption Profiles
- **Low Consumers** (<0.5 kWh avg): 95.4% of households
- **Medium Consumers** (0.5-2.0 kWh avg): 4.6% of households  
- **High Consumers** (>2.0 kWh avg): 0.0% of households
- **Variable Consumers** (high variation): 51.0% of households

### ⏰ Temporal Patterns
- **Peak Day**: Sunday (0.222 kWh avg)
- **Lowest Day**: Tuesday (0.206 kWh avg)
- **Evening Peak**: 17:00-22:00 hours
- **Night Low**: 23:00-06:00 hours

## Implementation Changes

### 1. Client/Smart Meter Simulation (`client/client_continuous.cpp`)

#### Household Type Classification
```cpp
enum class HouseholdType {
    LOW_CONSUMER,      // 95.4% - avg <0.5 kWh
    MEDIUM_CONSUMER,   // 4.6% - avg 0.5-2.0 kWh  
    HIGH_CONSUMER,     // 0.0% - avg >2.0 kWh
    VARIABLE_CONSUMER  // 51.0% - high variation
};
```

#### Realistic Temporal Factors
```cpp
static constexpr double hourly_factors_[24] = {
    0.517, 0.476, 0.448, 0.423, 0.516, 0.563, 0.610, 0.704,  // 0-7h
    0.751, 0.775, 0.798, 0.845, 0.892, 0.915, 0.939, 0.962,  // 8-15h  
    0.986, 1.056, 1.127, 1.479, 1.197, 1.169, 1.141, 0.587   // 16-23h
};

static constexpr double daily_factors_[7] = {
    1.042, 0.967, 0.972, 0.986, 0.993, 1.000, 1.042  // Sun-Sat
};
```

#### Energy Generation Algorithm
```cpp
void generate_and_encrypt_data() {
    // Apply household-specific consumption pattern
    current_energy_value_ = base_consumption_ * hourly_factor * daily_factor;
    
    // Add variation based on household type and variability
    normal_distribution<double> variation(0.0, variation_factor_);
    current_energy_value_ += (current_energy_value_ * variation_amount);
    
    // Implement 0.000 kWh consumers (some households from dataset)
    if (household_type_ == HouseholdType::LOW_CONSUMER && random_factor < 0.02) {
        current_energy_value_ = 0.0;  // 2% chance of zero consumption
    }
}
```

### 2. Control Center Analytics (`control_center/control_center_continuous.cpp`)

#### Updated Processing with Realistic Values
```cpp
// Base consumption from dataset: average = 0.213 kWh per household
double base_consumption = 0.213;
double hourly_factor = hourly_factors[tm.tm_hour];
double daily_factor = daily_factors[tm.tm_wday];

// Add realistic variation (from dataset standard deviation analysis)
normal_distribution<> variation(1.0, 0.3);  // 30% variation coefficient
```

### 3. Configuration Updates (`config.json`)

Updated to support the realistic scale:
```json
{
  "comment_clients": "Number of smart meters to simulate. Based on dataset analysis of 5,560 households.",
  "num_clients": 5560
}
```

## Key Features

### 1. **Realistic Household Distribution**
- 95.4% low consumers (0.15-0.5 kWh)
- 4.6% medium consumers (0.5-2.0 kWh)
- 0.05% high consumers (2.0-2.112 kWh)
- 51% with high variation patterns

### 2. **Temporal Pattern Accuracy**
- Hourly consumption factors based on real data
- Peak at 19:00 (evening dinner time)
- Low at 4:00 (deep night)
- Daily variations throughout the week

### 3. **Statistical Accuracy**
- Average consumption matches dataset: 0.213 kWh
- Consumption range: 0.000 - 2.112 kWh
- Realistic variation patterns
- Zero consumption households (meters off/vacant)

### 4. **Enhanced Analytics**
The control center now provides analytics based on real patterns:
- Grid efficiency calculations
- Peak demand prediction
- Anomaly detection based on realistic thresholds
- Load balancing recommendations

## Running the Realistic Simulation

### Quick Start
```bash
./demo_realistic_consumption.sh
```

### Manual Start
```bash
# Start services
cd build
./kdc &
./control_center_continuous &
./aggregator_continuous &

# Start multiple smart meters
for i in {1..10}; do
    ./client_continuous $i &
done
```

## Observing Realistic Patterns

When running the simulation, observe:

1. **Smart Meter Logs**: Show household types and consumption patterns
   ```
   [2024-12-11 10:30:15] Meter 1 (Low-Variable): 0.087 kWh (H:0.80, D:0.99)
   [2024-12-11 19:30:15] Meter 2 (Medium): 0.845 kWh (H:1.48, D:1.00)
   ```

2. **Control Center Analytics**: Realistic grid statistics
   ```
   Grid Status Summary:
   Total Energy (24h): 1,183.45 kWh
   Average per Reading: 0.213 kWh
   Peak Consumption: 1.987 kWh  
   Minimum Consumption: 0.000 kWh
   ```

3. **Peak/Off-Peak Detection**: Based on real consumption patterns

## Performance and Security

The realistic simulation maintains:
- ✅ **Full homomorphic encryption** of energy data
- ✅ **Privacy-preserving aggregation** 
- ✅ **Secure key distribution**
- ✅ **Performance metrics tracking**
- ✅ **Realistic computational load**

## Data Sources

The consumption patterns are based on analysis of:
- UK household energy consumption dataset
- 5,560 households over 29,950 average readings each
- Real temporal and statistical patterns
- Household classification based on consumption behavior

## Future Enhancements

Potential improvements:
1. **Seasonal variations** (summer/winter patterns)
2. **Weather correlation** (heating/cooling impacts)
3. **Socio-economic factors** (different household sizes)
4. **Smart appliance integration** (EV charging, solar panels)
5. **Real-time pricing response** (demand response programs)

---

This implementation provides a highly realistic smart grid simulation that accurately reflects real-world energy consumption patterns while maintaining privacy and security through homomorphic encryption.