# SHT3X Sensor Driver Library

Provides I2C-based communication and measurement control for SHT3X temperature and humidity sensors.

## Overview

The SHT3X Sensor Driver Library provides a complete I2C driver for the Sensirion SHT3X family of temperature and humidity sensors (SHT30, SHT31, SHT35). It supports both single-shot and periodic measurement modes with configurable repeatability settings.

## Files

- **sht3x.c**: SHT3X driver implementation
- **sht3x.h**: SHT3X API declarations and data structures

## Hardware Configuration

### SHT3X Sensor Specifications

- **Interface**: I2C (400 kHz Fast Mode supported)
- **Temperature Range**: -40°C to +125°C
- **Temperature Accuracy**: ±0.2°C (typical, SHT35)
- **Humidity Range**: 0% to 100% RH
- **Humidity Accuracy**: ±1.5% RH (typical, SHT35)
- **Resolution**: 14-bit temperature, 14-bit humidity
- **Supply Voltage**: 2.4V to 5.5V

### I2C Addresses

```c
#define SHT3X_I2C_ADDR_GND  (0x44 << 1)  // ADDR pin connected to GND (default)
#define SHT3X_I2C_ADDR_VDD  (0x45 << 1)  // ADDR pin connected to VDD
```

**Current Configuration**: 0x44 (ADDR pin to GND)

**7-bit vs 8-bit**: STM32 HAL uses 8-bit addresses (7-bit address << 1).

### Pin Connections

| STM32 Pin | SHT3X Pin | Description                             |
| --------- | --------- | --------------------------------------- |
| I2C1_SCL  | SCL       | Serial Clock                            |
| I2C1_SDA  | SDA       | Serial Data                             |
| 3.3V      | VDD       | Power Supply                            |
| GND       | VSS, ADDR | Ground (ADDR tied low for address 0x44) |

## Data Structure

### SHT3X Device Structure

```c
typedef struct {
    I2C_HandleTypeDef *hi2c;     // I2C handle pointer
    uint8_t device_address;      // 8-bit I2C address
    float temperature;           // Last measured temperature (°C)
    float humidity;              // Last measured humidity (%RH)
    sht3x_mode_t currentState;   // Current operating mode
    sht3x_repeat_t modeRepeat;   // Current repeatability setting
} sht3x_t;
```

**Global Instance**:

```c
extern sht3x_t g_sht3x;
```

## Operating Modes

### Mode Enumeration

```c
typedef enum {
    SHT3X_IDLE = 0,           // Initial state, no measurement
    SHT3X_SINGLE_SHOT,        // One-time measurement on demand
    SHT3X_PERIODIC_05MPS,     // 0.5 measurements per second
    SHT3X_PERIODIC_1MPS,      // 1 measurement per second
    SHT3X_PERIODIC_2MPS,      // 2 measurements per second
    SHT3X_PERIODIC_4MPS,      // 4 measurements per second
    SHT3X_PERIODIC_10MPS      // 10 measurements per second
} sht3x_mode_t;
```

**Mode Descriptions**:

- **IDLE**: No measurements, sensor in standby
- **SINGLE_SHOT**: On-demand measurement, sensor returns to idle after completion
- **PERIODIC_xMPS**: Continuous measurements at specified rate

### Repeatability Settings

```c
typedef enum {
    SHT3X_HIGH = 0,    // High repeatability (best accuracy, longest time)
    SHT3X_MEDIUM,      // Medium repeatability (balanced)
    SHT3X_LOW          // Low repeatability (fastest, lower accuracy)
} sht3x_repeat_t;
```

**Measurement Duration**:
| Repeatability | Duration | Use Case |
|---------------|----------|---------------------------------|
| HIGH | 15.5 ms | Best accuracy, power available |
| MEDIUM | 6.5 ms | Balanced performance |
| LOW | 4.5 ms | Fast updates, power constrained |

## API Functions

### Initialization

```c
void SHT3X_Init(sht3x_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr7bit);
```

Initializes the SHT3X sensor.

**Parameters**:

- `dev`: Pointer to SHT3X device structure
- `hi2c`: Pointer to I2C handle (e.g., &hi2c1)
- `addr7bit`: 7-bit I2C address (0x44 or 0x45)

**Usage Example**:

```c
extern I2C_HandleTypeDef hi2c1;
sht3x_t sensor;

void SensorInit(void)
{
    SHT3X_Init(&sensor, &hi2c1, 0x44);
    // Sensor initialized to IDLE state
}
```

### De-initialization

```c
void SHT3X_DeInit(sht3x_t *dev);
```

De-initializes the sensor (stops periodic measurements if active).

**Usage Example**:

```c
SHT3X_DeInit(&sensor);
```

### Single-Shot Measurement

```c
SHT3X_StatusTypeDef SHT3X_Single(sht3x_t *dev, sht3x_repeat_t *modeRepeat,
                                 float *outT, float *outRH);
```

Performs a single on-demand measurement.

**Parameters**:

- `dev`: Pointer to SHT3X device structure
- `modeRepeat`: Pointer to repeatability setting (HIGH, MEDIUM, or LOW)
- `outT`: Pointer to store temperature result (can be NULL)
- `outRH`: Pointer to store humidity result (can be NULL)

**Returns**:

- `SHT3X_OK`: Measurement successful
- `SHT3X_ERROR`: I2C communication error or CRC error

**Usage Example**:

```c
float temperature, humidity;
sht3x_repeat_t repeatability = SHT3X_HIGH;

if (SHT3X_Single(&sensor, &repeatability, &temperature, &humidity) == SHT3X_OK)
{
    printf("Temp: %.2f C, Hum: %.2f %%\n", temperature, humidity);
}
else
{
    printf("Sensor read error\n");
}
```

**Measurement Time**: 4.5-15.5 ms (depends on repeatability)

### Periodic Measurement Start

```c
SHT3X_StatusTypeDef SHT3X_Periodic(sht3x_t *dev, sht3x_mode_t *modePeriodic,
                                   sht3x_repeat_t *modeRepeat,
                                   float *outT, float *outRH);
```

Starts periodic measurements at specified rate.

**Parameters**:

- `dev`: Pointer to SHT3X device structure
- `modePeriodic`: Pointer to periodic mode (05MPS, 1MPS, 2MPS, 4MPS, 10MPS)
- `modeRepeat`: Pointer to repeatability setting
- `outT`: Pointer to store temperature (can be NULL)
- `outRH`: Pointer to store humidity (can be NULL)

**Returns**:

- `SHT3X_OK`: Periodic mode started successfully
- `SHT3X_ERROR`: I2C error

**Usage Example**:

```c
sht3x_mode_t mode = SHT3X_PERIODIC_1MPS;
sht3x_repeat_t repeatability = SHT3X_HIGH;
float temp, hum;

if (SHT3X_Periodic(&sensor, &mode, &repeatability, &temp, &hum) == SHT3X_OK)
{
    printf("Periodic mode started at 1 MPS\n");
    printf("Initial: Temp=%.2f C, Hum=%.2f %%\n", temp, hum);
}
```

**Note**: Sensor continues measurements automatically after start.

### Fetch Periodic Data

```c
void SHT3X_FetchData(sht3x_t *dev, float *outT, float *outRH);
```

Fetches the latest measurement from periodic mode (without triggering new measurement).

**Parameters**:

- `dev`: Pointer to SHT3X device structure
- `outT`: Pointer to store temperature
- `outRH`: Pointer to store humidity

**Usage Example**:

```c
// In periodic mode (e.g., 1 MPS)
while (1)
{
    HAL_Delay(1000);  // Wait 1 second

    float temp, hum;
    SHT3X_FetchData(&sensor, &temp, &hum);
    printf("Temp: %.2f C, Hum: %.2f %%\n", temp, hum);
}
```

**Important**: Must be called after periodic mode is started.

### Stop Periodic Measurements

```c
SHT3X_StatusTypeDef SHT3X_Stop_Periodic(sht3x_t *dev);
```

Stops periodic measurements and returns sensor to idle state.

**Returns**:

- `SHT3X_OK`: Stopped successfully
- `SHT3X_ERROR`: I2C error

**Usage Example**:

```c
if (SHT3X_Stop_Periodic(&sensor) == SHT3X_OK)
{
    printf("Periodic mode stopped\n");
}
```

### Heater Control

```c
SHT3X_StatusTypeDef SHT3X_Heater(sht3x_t *dev, const sht3x_heater_mode_t *modeHeater);
```

Enables or disables the internal heater.

**Parameters**:

- `dev`: Pointer to SHT3X device structure
- `modeHeater`: Pointer to heater mode (ENABLE or DISABLE)

**Returns**:

- `SHT3X_OK`: Heater control successful
- `SHT3X_ERROR`: I2C error

**Usage Example**:

```c
sht3x_heater_mode_t heater_on = SHT3X_HEATER_ENABLE;
SHT3X_Heater(&sensor, &heater_on);

HAL_Delay(5000);  // Heat for 5 seconds

sht3x_heater_mode_t heater_off = SHT3X_HEATER_DISABLE;
SHT3X_Heater(&sensor, &heater_off);
```

**Heater Purpose**:

- Remove condensation from sensor
- Verify sensor functionality (temperature should increase)
- **Warning**: Long-term heater use reduces sensor lifetime

### Accelerated Response Time (ART)

```c
SHT3X_StatusTypeDef SHT3X_ART(sht3x_t *dev);
```

Enables accelerated response time mode (faster measurement stabilization).

**Returns**:

- `SHT3X_OK`: ART enabled
- `SHT3X_ERROR`: I2C error

**Usage Example**:

```c
SHT3X_ART(&sensor);
// Sensor now responds faster to environmental changes
```

**Use Case**: Applications requiring fast response to rapid environmental changes.

## CRC Validation

### Built-in CRC Checking

All sensor readings include CRC-8 checksum for data integrity verification.

**CRC Polynomial**: 0x31 (x^8 + x^5 + x^4 + 1)
**Initial Value**: 0xFF

**Automatic Validation**: Driver automatically validates CRC and returns error if mismatch detected.

**Error Handling**:

```c
SHT3X_StatusTypeDef result = SHT3X_Single(&sensor, &repeat, &temp, &hum);
if (result == SHT3X_ERROR)
{
    // Could be I2C error or CRC error
    printf("Sensor error (I2C or CRC)\n");
}
```

## Data Conversion

### Temperature Conversion

Raw sensor data is 16-bit unsigned integer.

**Conversion Formula**:

```
Temperature (°C) = -45 + (175 × raw_value / 65535)
```

**Example**:

```
Raw value: 32768 (0x8000)
Temperature = -45 + (175 × 32768 / 65535) = -45 + 87.5 = 42.5°C
```

### Humidity Conversion

**Conversion Formula**:

```
Humidity (%RH) = 100 × raw_value / 65535
```

**Example**:

```
Raw value: 32768 (0x8000)
Humidity = 100 × 32768 / 65535 = 50.0%
```

## Usage Examples

### Single-Shot Mode (SINGLE Command)

```c
void SINGLE_PARSER(uint8_t argc, char **argv)
{
    float temp, hum;
    sht3x_repeat_t repeatability = SHT3X_HIGH;

    if (SHT3X_Single(&g_sht3x, &repeatability, &temp, &hum) == SHT3X_OK)
    {
        // Update data manager
        DATA_MANAGER_Update_Sensor(temp, hum);

        // Send JSON output
        sensor_json_output_send("SINGLE", temp, hum);

        PRINT_CLI("OK: Temp=%.2f C, Hum=%.2f %%\r\n", temp, hum);
    }
    else
    {
        PRINT_CLI("ERROR: Sensor read failed\r\n");
    }
}
```

### Periodic Mode with Timer

```c
// Start periodic mode
void PERIODIC_ON_PARSER(uint8_t argc, char **argv)
{
    sht3x_mode_t mode = SHT3X_PERIODIC_1MPS;
    sht3x_repeat_t repeat = SHT3X_HIGH;
    float temp, hum;

    if (SHT3X_Periodic(&g_sht3x, &mode, &repeat, &temp, &hum) == SHT3X_OK)
    {
        DATA_MANAGER_Set_Mode(PERIODIC_MODE);
        PRINT_CLI("Periodic mode started\r\n");

        // Start STM32 timer for periodic reads
        HAL_TIM_Base_Start_IT(&htim2);
    }
}

// Timer interrupt handler
void TIM2_IRQHandler(void)
{
    if (/* timer interrupt flag */)
    {
        Flag_Periodic = 1;  // Signal main loop
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    }
}

// Main loop periodic handler
void MainLoop_Periodic(void)
{
    if (Flag_Periodic)
    {
        Flag_Periodic = 0;

        float temp, hum;
        SHT3X_FetchData(&g_sht3x, &temp, &hum);

        DATA_MANAGER_Update_Sensor(temp, hum);
        sensor_json_output_send("PERIODIC", temp, hum);

        // Log to SD card
        SDCardManager_WriteData(0, temp, hum, "PERIODIC");
    }
}
```

### Error Recovery

```c
uint8_t retry_count = 0;
const uint8_t MAX_RETRIES = 3;

while (retry_count < MAX_RETRIES)
{
    if (SHT3X_Single(&sensor, &repeat, &temp, &hum) == SHT3X_OK)
    {
        // Success
        break;
    }
    else
    {
        retry_count++;
        HAL_Delay(10);  // Wait before retry
    }
}

if (retry_count == MAX_RETRIES)
{
    printf("Sensor communication failed after %d retries\n", MAX_RETRIES);
    // Try sensor reset or re-initialization
}
```

## Performance Characteristics

### Measurement Times

| Repeatability | Single-Shot Time | Power Consumption |
| ------------- | ---------------- | ----------------- |
| HIGH          | 15.5 ms          | 1.5 mA            |
| MEDIUM        | 6.5 ms           | 0.8 mA            |
| LOW           | 4.5 ms           | 0.5 mA            |

### Periodic Mode Current

| Measurement Rate | Average Current |
| ---------------- | --------------- |
| 0.5 MPS          | ~100 µA         |
| 1 MPS            | ~150 µA         |
| 2 MPS            | ~250 µA         |
| 4 MPS            | ~450 µA         |
| 10 MPS           | ~1000 µA        |

### I2C Transaction

Typical I2C read sequence:

1. Send measurement command: ~100 µs
2. Wait for measurement: 4.5-15.5 ms
3. Read 6 bytes (2T + 1CRC + 2RH + 1CRC): ~200 µs

**Total**: 5-16 ms depending on repeatability

## Power Optimization

### Sleep Mode

When not in use, sensor enters low-power sleep:

```c
SHT3X_Stop_Periodic(&sensor);  // Stop measurements
// Sensor now in low-power mode (~0.2 µA)
```

### Periodic Mode Selection

For battery-powered applications:

- **0.5 MPS**: Best power efficiency
- **1 MPS**: Good balance
- **10 MPS**: High power, use only when needed

### Low Repeatability Mode

For power-constrained systems:

```c
sht3x_repeat_t repeat = SHT3X_LOW;  // Fastest, lowest power
SHT3X_Single(&sensor, &repeat, &temp, &hum);
```

## Error Handling

### I2C Timeout

```c
#define SHT3X_I2C_TIMEOUT 100  // 100 ms timeout
```

If I2C communication exceeds timeout:

- Function returns `SHT3X_ERROR`
- Check I2C bus for physical issues

### CRC Error

If CRC validation fails:

- Function returns `SHT3X_ERROR`
- Indicates data corruption
- Retry measurement

### Sensor Not Responding

```c
// Check sensor presence
sht3x_repeat_t repeat = SHT3X_LOW;
if (SHT3X_Single(&sensor, &repeat, NULL, NULL) != SHT3X_OK)
{
    printf("Sensor not detected on I2C bus\n");
    // Check wiring, I2C address, power supply
}
```

## Best Practices

### 1. Use Appropriate Repeatability

```c
// For accurate data logging: HIGH
sht3x_repeat_t repeat = SHT3X_HIGH;

// For fast updates: LOW
sht3x_repeat_t repeat = SHT3X_LOW;
```

### 2. Wait for Measurement Completion

```c
// Don't poll too fast in single-shot mode
SHT3X_Single(&sensor, &repeat, &temp, &hum);
// Driver handles wait internally
```

### 3. Stop Periodic Mode Before Reconfiguration

```c
SHT3X_Stop_Periodic(&sensor);
// Now safe to change modes or enter low power
```

### 4. Validate Sensor Data

```c
if (temperature < -40.0 || temperature > 125.0)
{
    printf("Temperature out of range\n");
}
if (humidity < 0.0 || humidity > 100.0)
{
    printf("Humidity out of range\n");
}
```

## Memory Footprint

### RAM Usage

- Device structure: 20 bytes
- Stack during measurement: ~50 bytes
- **Total**: ~70 bytes

### Flash Usage

- Driver code: ~2-3 KB

## Dependencies

### Required

- **STM32 HAL I2C**: I2C communication
- **math.h**: Floating-point calculations (optional, for conversions)

### Used By

- **cmd_parser.c**: SINGLE command handler
- **data_manager.c**: Sensor data storage
- **main.c**: Periodic measurement loop

## Summary

The SHT3X Sensor Driver Library provides:

- Complete I2C driver for SHT30/SHT31/SHT35 sensors
- Single-shot and periodic measurement modes (0.5-10 MPS)
- Three repeatability levels (HIGH, MEDIUM, LOW)
- Built-in CRC validation for data integrity
- Temperature range: -40°C to +125°C (±0.2°C accuracy)
- Humidity range: 0-100% RH (±1.5% accuracy)
- Low power operation (~0.2 µA sleep, ~100 µA @ 0.5 MPS)
- Heater control for condensation removal

This driver enables accurate environmental monitoring for the Datalogger system with flexible measurement modes and excellent power efficiency.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
