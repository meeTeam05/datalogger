# Data Manager Library

Manages centralized data storage and system state for sensors and operating modes.

## Overview

The Data Manager Library provides centralized sensor data storage and state management for the Datalogger system. It maintains current sensor readings (temperature and humidity), operational modes (SINGLE or PERIODIC), and provides thread-safe access to this data for all system components.

## Files

- **data_manager.c**: Data manager implementation
- **data_manager.h**: Data manager API and structure definitions

## Core Data Structure

### SensorData Structure

```c
typedef struct {
    float temperature;    // Current temperature in Celsius
    float humidity;       // Current humidity in %RH
    uint8_t mode;        // Operating mode: SINGLE or PERIODIC
} SensorData;
```

### Global Instance

```c
SensorData sensorData = {0.0f, 0.0f, SINGLE_MODE};
```

**Initial Values**:

- Temperature: 0.0°C
- Humidity: 0.0%
- Mode: SINGLE_MODE (default)

## Operating Modes

### Mode Constants

```c
#define SINGLE_MODE    0    // On-demand measurement mode
#define PERIODIC_MODE  1    // Continuous measurement mode
```

### Mode Descriptions

**SINGLE_MODE (0)**:

- Default operating mode
- Measurements triggered by UART command "SINGLE"
- One measurement per command
- No automatic readings
- Lower power consumption

**PERIODIC_MODE (1)**:

- Automatic measurement mode
- Measurements triggered by TIM2 interrupt
- Interval configurable: 5-3600 seconds
- Continuous operation
- Automatic JSON output and SD card logging

## API Functions

### Data Update Functions

#### Update Temperature

```c
void DATA_MANAGER_Update_Temperature(float temp);
```

Updates the stored temperature value.

**Parameters**:

- `temp`: Temperature in Celsius (-40.0 to +125.0 for SHT3X)

**Usage Example**:

```c
float temperature = 25.5f;
DATA_MANAGER_Update_Temperature(temperature);
```

**Thread Safety**: Safe to call from any context (ISR or main loop)

#### Update Humidity

```c
void DATA_MANAGER_Update_Humidity(float humi);
```

Updates the stored humidity value.

**Parameters**:

- `humi`: Relative humidity in %RH (0.0 to 100.0)

**Usage Example**:

```c
float humidity = 65.2f;
DATA_MANAGER_Update_Humidity(humidity);
```

**Thread Safety**: Safe to call from any context (ISR or main loop)

#### Update Both Temperature and Humidity

```c
void DATA_MANAGER_Update_Sensor(float temp, float humi);
```

Updates both sensor values atomically.

**Parameters**:

- `temp`: Temperature in Celsius
- `humi`: Relative humidity in %RH

**Usage Example**:

```c
// After SHT3X measurement
float temp = 23.8f;
float hum = 58.4f;
DATA_MANAGER_Update_Sensor(temp, hum);
```

**Advantage**: Ensures temperature and humidity are from the same measurement cycle.

**Typical Use Case**: Called after successful SHT3X sensor read.

### Mode Management Functions

#### Set Operating Mode

```c
void DATA_MANAGER_Set_Mode(uint8_t newMode);
```

Changes the operating mode.

**Parameters**:

- `newMode`: SINGLE_MODE (0) or PERIODIC_MODE (1)

**Usage Example**:

```c
// Switch to periodic mode
DATA_MANAGER_Set_Mode(PERIODIC_MODE);

// Switch back to single mode
DATA_MANAGER_Set_Mode(SINGLE_MODE);
```

**Side Effects**: None. Mode change does not automatically start/stop timers. Timer control is handled separately by command parsers.

### Data Retrieval Functions

#### Get Temperature

```c
float DATA_MANAGER_Get_Temperature(void);
```

Retrieves the current stored temperature.

**Returns**: Temperature in Celsius

**Usage Example**:

```c
float currentTemp = DATA_MANAGER_Get_Temperature();
printf("Temperature: %.2f C\n", currentTemp);
```

#### Get Humidity

```c
float DATA_MANAGER_Get_Humidity(void);
```

Retrieves the current stored humidity.

**Returns**: Humidity in %RH

**Usage Example**:

```c
float currentHum = DATA_MANAGER_Get_Humidity();
printf("Humidity: %.2f %%\n", currentHum);
```

#### Get Operating Mode

```c
uint8_t DATA_MANAGER_Get_Mode(void);
```

Retrieves the current operating mode.

**Returns**: SINGLE_MODE (0) or PERIODIC_MODE (1)

**Usage Example**:

```c
uint8_t mode = DATA_MANAGER_Get_Mode();
if (mode == PERIODIC_MODE)
{
    printf("Periodic mode active\n");
}
else
{
    printf("Single mode active\n");
}
```

#### Get Complete Sensor Data

```c
SensorData DATA_MANAGER_Get_SensorData(void);
```

Retrieves the entire sensor data structure.

**Returns**: Copy of SensorData structure (by value)

**Usage Example**:

```c
SensorData data = DATA_MANAGER_Get_SensorData();
printf("Temp: %.2f C, Hum: %.2f %%, Mode: %d\n",
       data.temperature,
       data.humidity,
       data.mode);
```

**Advantage**: Atomic read of all sensor data ensures consistency.

## Data Flow Integration

### Single Mode Flow

```
1. UART Command "SINGLE" received
2. SINGLE_PARSER() executes
3. SHT3X_Read_TempHumi(&temp, &humi)
4. DATA_MANAGER_Update_Sensor(temp, humi)
5. SENSOR_JSON_OUTPUT() reads data
6. JSON transmitted via UART
```

### Periodic Mode Flow

```
1. UART Command "PERIODIC ON" received
2. PERIODIC_ON_PARSER() executes
3. DATA_MANAGER_Set_Mode(PERIODIC_MODE)
4. Start TIM2 timer

[Every interval:]
5. TIM2 interrupt triggers
6. SHT3X_Read_TempHumi(&temp, &humi)
7. DATA_MANAGER_Update_Sensor(temp, humi)
8. SENSOR_JSON_OUTPUT() reads data
9. JSON transmitted via UART
10. SD_CARD_MANAGER_Write(jsonBuffer)
```

### Display Update Flow

```
[Main Loop - Every 500ms:]
1. DISPLAY_Update() called
2. DATA_MANAGER_Get_Temperature()
3. DATA_MANAGER_Get_Humidity()
4. ILI9225_DrawText() with values
5. LCD shows real-time data
```

## Usage Examples

### Complete Measurement Cycle

```c
void MeasurementTask(void)
{
    float temperature, humidity;

    // Read sensor
    if (SHT3X_Read_TempHumi(&temperature, &humidity) == 0)
    {
        // Update data manager
        DATA_MANAGER_Update_Sensor(temperature, humidity);

        // Output as JSON
        SENSOR_JSON_OUTPUT();

        // Log to SD card if in periodic mode
        if (DATA_MANAGER_Get_Mode() == PERIODIC_MODE)
        {
            char jsonBuffer[256];
            snprintf(jsonBuffer, sizeof(jsonBuffer),
                     "{\"temp\":%.2f,\"hum\":%.2f}",
                     temperature, humidity);
            SD_CARD_MANAGER_Write(jsonBuffer);
        }
    }
}
```

### Mode-Dependent Behavior

```c
void ProcessSensorData(void)
{
    uint8_t currentMode = DATA_MANAGER_Get_Mode();

    if (currentMode == SINGLE_MODE)
    {
        // One-time operation
        printf("Single measurement completed\n");
    }
    else  // PERIODIC_MODE
    {
        // Continuous operation
        printf("Periodic measurement %d\n", measurementCount++);

        // Additional periodic tasks
        UpdateSDCard();
        TransmitToESP32();
    }
}
```

### Display Integration

```c
void UpdateLCDDisplay(void)
{
    char buffer[32];

    // Get current sensor data
    float temp = DATA_MANAGER_Get_Temperature();
    float hum = DATA_MANAGER_Get_Humidity();
    uint8_t mode = DATA_MANAGER_Get_Mode();

    // Display temperature
    snprintf(buffer, sizeof(buffer), "Temp: %.1f C", temp);
    ILI9225_DrawText(10, 50, buffer, COLOR_WHITE);

    // Display humidity
    snprintf(buffer, sizeof(buffer), "Hum: %.1f %%", hum);
    ILI9225_DrawText(10, 70, buffer, COLOR_WHITE);

    // Display mode
    const char *modeStr = (mode == PERIODIC_MODE) ? "PERIODIC" : "SINGLE";
    ILI9225_DrawText(10, 90, modeStr, COLOR_GREEN);
}
```

## Thread Safety Considerations

### Atomic Operations

All float assignments and reads on Cortex-M3 (STM32F103) are atomic:

- 32-bit float loads/stores are single instruction
- No need for critical sections for simple read/write

### Non-Atomic Scenarios

Reading multiple fields requires care:

**UNSAFE**:

```c
// These two reads may see inconsistent state
float temp = sensorData.temperature;
float hum = sensorData.humidity;
```

**SAFE**:

```c
// Use getter that returns entire structure
SensorData data = DATA_MANAGER_Get_SensorData();
float temp = data.temperature;
float hum = data.humidity;
```

### ISR-Safe Updates

Safe to call from TIM2 interrupt:

```c
void TIM2_IRQHandler(void)
{
    if (/* timer interrupt */)
    {
        float temp, hum;
        SHT3X_Read_TempHumi(&temp, &hum);

        // Safe: single structure update
        DATA_MANAGER_Update_Sensor(temp, hum);
    }
}
```

## Data Validation

The library does NOT perform validation. Caller must validate:

### Temperature Range Validation

```c
#define TEMP_MIN  -40.0f
#define TEMP_MAX  125.0f

void SafeUpdateTemperature(float temp)
{
    if (temp >= TEMP_MIN && temp <= TEMP_MAX)
    {
        DATA_MANAGER_Update_Temperature(temp);
    }
    else
    {
        printf("Invalid temperature: %.2f\n", temp);
    }
}
```

### Humidity Range Validation

```c
#define HUM_MIN  0.0f
#define HUM_MAX  100.0f

void SafeUpdateHumidity(float hum)
{
    if (hum >= HUM_MIN && hum <= HUM_MAX)
    {
        DATA_MANAGER_Update_Humidity(hum);
    }
    else
    {
        printf("Invalid humidity: %.2f\n", hum);
    }
}
```

### Mode Validation

```c
void SafeSetMode(uint8_t mode)
{
    if (mode == SINGLE_MODE || mode == PERIODIC_MODE)
    {
        DATA_MANAGER_Set_Mode(mode);
    }
    else
    {
        printf("Invalid mode: %d\n", mode);
    }
}
```

## Memory Footprint

### RAM Usage

```
SensorData structure: 9 bytes
  - float temperature: 4 bytes
  - float humidity: 4 bytes
  - uint8_t mode: 1 byte
```

### Flash Usage

- Function code: ~200 bytes
- Inline functions: 0 bytes (compiled inline)

### Total System Impact

Negligible: < 0.1% of STM32F103C8T6 resources (20KB RAM, 64KB Flash)

## Performance Characteristics

### Function Call Overhead

All functions are lightweight:

- Update functions: 1-2 instructions (direct assignment)
- Get functions: 1 instruction (direct read)
- Execution time: < 1 microsecond

### No Dynamic Allocation

All data is statically allocated at compile time:

- No malloc/free
- No heap fragmentation
- Deterministic memory usage

## Best Practices

### Centralized Data Access

Always use Data Manager instead of direct sensor reads:

**WRONG**:

```c
// Don't read sensor multiple times
float temp1 = SHT3X_Read_Temperature();
// ... some code ...
float temp2 = SHT3X_Read_Temperature();  // Wasteful
```

**CORRECT**:

```c
// Read once, store in Data Manager
DATA_MANAGER_Update_Sensor(temp, hum);

// Access cached data multiple times
float temp1 = DATA_MANAGER_Get_Temperature();
// ... some code ...
float temp2 = DATA_MANAGER_Get_Temperature();  // Efficient
```

### Mode-Aware Programming

Check mode before performing mode-specific operations:

```c
void ProcessData(void)
{
    // Common processing
    float temp = DATA_MANAGER_Get_Temperature();

    // Mode-specific processing
    if (DATA_MANAGER_Get_Mode() == PERIODIC_MODE)
    {
        // Only log to SD card in periodic mode
        LogToSDCard(temp);
    }
}
```

### Initialization

Initialize data manager before use:

```c
void SystemInit(void)
{
    // Set default mode
    DATA_MANAGER_Set_Mode(SINGLE_MODE);

    // Set initial values (optional)
    DATA_MANAGER_Update_Sensor(0.0f, 0.0f);

    // Initialize other components
    SHT3X_Init();
    SD_CARD_MANAGER_Init();
}
```

## Dependencies

### Used By

This library is used by:

- **cmd_parser**: Command handlers read/write mode and sensor data
- **sensor_json_output**: Reads sensor data for JSON serialization
- **display**: Reads sensor data for LCD updates
- **main loop**: Reads mode for conditional processing

### Uses

This library uses:

- **None** - No external dependencies (pure data storage)

## Integration Example

Complete integration in main.c:

```c
int main(void)
{
    // System initialization
    HAL_Init();
    SystemClock_Config();

    // Initialize peripherals
    MX_USART1_UART_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();

    // Initialize sensors
    SHT3X_Init();

    // Initialize data manager
    DATA_MANAGER_Set_Mode(SINGLE_MODE);
    DATA_MANAGER_Update_Sensor(0.0f, 0.0f);

    // Main loop
    while (1)
    {
        // Handle UART commands
        if (Flag_UART)
        {
            Flag_UART = 0;
            COMMAND_EXECUTE((char *)buff);
        }

        // Update display every 500ms
        static uint32_t lastUpdate = 0;
        if (HAL_GetTick() - lastUpdate > 500)
        {
            lastUpdate = HAL_GetTick();
            DISPLAY_Update();  // Reads from DATA_MANAGER
        }

        // Periodic mode handling
        if (Flag_Periodic)
        {
            Flag_Periodic = 0;

            // Read sensor
            float temp, hum;
            SHT3X_Read_TempHumi(&temp, &hum);

            // Update data manager
            DATA_MANAGER_Update_Sensor(temp, hum);

            // Output JSON
            SENSOR_JSON_OUTPUT();

            // Log to SD card
            SD_CARD_MANAGER_Write_Sensor();
        }
    }
}
```

## Summary

The Data Manager Library provides a simple, efficient, and thread-safe mechanism for:

- Centralized sensor data storage
- Operating mode management
- Consistent data access across system components
- Zero-overhead data synchronization

This design pattern eliminates data duplication, reduces coupling between modules, and ensures all components see consistent sensor readings and operational state.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
