# Sensor JSON Output Library

Generates and transmits sensor data in JSON format for external communication.

## Overview

The Sensor JSON Output Library provides JSON serialization for sensor data and automatic transmission over UART. It formats temperature, humidity, timestamp, and mode information into standardized JSON strings for communication with the ESP32 module and web dashboard.

## Files

- **sensor_json_output.c**: JSON formatting and output implementation
- **sensor_json_output.h**: JSON output API declarations

## Core Functionality

### JSON Format Specification

The library generates JSON strings in the following format:

```json
{
  "mode": "PERIODIC",
  "timestamp": 1710000000,
  "temperature": 25.5,
  "humidity": 65.2
}
```

**Field Specifications**:

- `timestamp`: Unix timestamp (seconds since January 1, 1970, 00:00:00 UTC)
- `temperature`: Floating-point value in Celsius, 2 decimal places
- `humidity`: Floating-point value in percentage, 2 decimal places
- `mode`: String literal, either "SINGLE" or "PERIODIC"

## API Functions

### Format JSON String (Buffer)

```c
int sensor_json_format(char *buffer, size_t buffer_size,
                       const char *mode, float temperature, float humidity,
                       uint32_t timestamp);
```

Formats sensor data into a JSON string without sending it.

**Parameters**:

- `buffer`: Pointer to destination buffer
- `buffer_size`: Size of the destination buffer
- `mode`: Mode string ("SINGLE" or "PERIODIC")
- `temperature`: Temperature in Celsius
- `humidity`: Humidity in percentage
- `timestamp`: Unix timestamp (if 0, reads from RTC)

**Returns**:

- **Positive**: Number of characters written (excluding null terminator)
- **-1**: Error (buffer too small or other error)

**Usage Example**:

```c
char json_buffer[256];
int len = sensor_json_format(json_buffer, sizeof(json_buffer),
                              "PERIODIC", 25.5, 65.2, 1710000000);

if (len > 0)
{
    printf("JSON: %s\n", json_buffer);
    // Output: {"mode":"PERIODIC","timestamp":1710000000,"temperature":25.50,"humidity":65.20}
}
else
{
    printf("JSON formatting failed\n");
}
```

**Buffer Size Requirement**: Minimum 100 bytes recommended (typical JSON length: 80-90 bytes).

### Format and Send via UART

```c
void sensor_json_output_send(const char *mode, float temperature, float humidity);
```

Formats sensor data into JSON and transmits via UART.

**Parameters**:

- `mode`: Mode string ("SINGLE" or "PERIODIC")
- `temperature`: Temperature in Celsius
- `humidity`: Humidity in percentage

**Behavior**:

1. Reads current timestamp from RTC (DS3231)
2. Formats JSON string in internal static buffer
3. Transmits via UART using `PRINT_CLI()`

**Usage Example**:

```c
// Get sensor data
float temp = DATA_MANAGER_Get_Temperature();
float hum = DATA_MANAGER_Get_Humidity();

// Send as JSON
sensor_json_output_send("PERIODIC", temp, hum);
```

**Typical Output**:

```
{"mode":"PERIODIC","timestamp":1710000000,"temperature":25.50,"humidity":65.20}
```

## Timestamp Handling

### Automatic RTC Reading

If `timestamp` parameter is 0, the function automatically reads the current time from the DS3231 RTC:

```c
uint32_t timestamp = 0;  // Auto-read from RTC
sensor_json_format(buffer, size, "SINGLE", 25.5, 65.2, timestamp);
```

**RTC Read Process**:

1. Read time: `DS3231_ReadTime(&time)`
2. Read date: `DS3231_ReadDate(&date)`
3. Convert to Unix timestamp:
   ```c
   struct tm timeinfo;
   timeinfo.tm_year = date.year - 1900;
   timeinfo.tm_mon = date.month - 1;
   timeinfo.tm_mday = date.date;
   timeinfo.tm_hour = time.hour;
   timeinfo.tm_min = time.minute;
   timeinfo.tm_sec = time.second;
   timestamp = mktime(&timeinfo);
   ```

### Manual Timestamp

For custom timestamps (e.g., from buffered data):

```c
uint32_t specific_time = 1710000000;
sensor_json_format(buffer, size, "PERIODIC", 25.5, 65.2, specific_time);
```

## JSON String Format Details

### Compact Format (No Whitespace)

The generated JSON is **compact** (no spaces or newlines):

```json
{
  "mode": "PERIODIC",
  "timestamp": 1710000000,
  "temperature": 25.5,
  "humidity": 65.2
}
```

**Rationale**:

- Smaller transmission size
- Faster UART transmission
- Bandwidth efficiency

**Typical Length**: 80-90 bytes

### Floating Point Precision

Temperature and humidity are formatted with **2 decimal places**:

```c
"temperature":25.50  // Always 2 decimals
"humidity":65.20     // Always 2 decimals
```

**Format Specifier**: `%.2f`

**Range Examples**:

```
Temperature: -40.00 to  125.00
Humidity: 0.00 to  100.00
Timestamp: 0 to 2147483647 (32-bit limit: year 2038)
```

### Mode String Values

Only two valid mode strings:

- `"SINGLE"`: On-demand measurement mode
- `"PERIODIC"`: Continuous monitoring mode

**Validation**: Caller must ensure correct mode string is passed.

## Buffer Management

### Internal Static Buffer

`sensor_json_output_send()` uses an internal static buffer:

```c
static char json_buffer[256];
```

**Size**: 256 bytes
**Scope**: Static (preserved between calls)

**Thread Safety**: **NOT thread-safe**. Do not call from multiple threads or ISRs.

### External Buffer Usage

For custom buffer management, use `sensor_json_format()`:

```c
// SD card buffering
char sd_buffer[512];
sensor_json_format(sd_buffer, sizeof(sd_buffer), mode, temp, hum, timestamp);
SD_CARD_MANAGER_Write(sd_buffer);

// ESP32 transmission
char uart_buffer[128];
sensor_json_format(uart_buffer, sizeof(uart_buffer), mode, temp, hum, 0);
PRINT_CLI("%s\r\n", uart_buffer);
```

## Usage Examples

### Single Mode Measurement

```c
void SINGLE_PARSER(uint8_t argc, char **argv)
{
    float temp, hum;

    // Read sensor
    if (SHT3X_Read_TempHumi(&temp, &hum) == 0)
    {
        // Update data manager
        DATA_MANAGER_Update_Sensor(temp, hum);

        // Send JSON output
        sensor_json_output_send("SINGLE", temp, hum);
    }
    else
    {
        PRINT_CLI("Sensor read error\r\n");
    }
}
```

**Output**:

```
{"mode":"SINGLE","timestamp":1710000000,"temperature":25.50,"humidity":65.20}
```

### Periodic Mode Measurement

```c
void TIM2_Periodic_Handler(void)
{
    float temp = DATA_MANAGER_Get_Temperature();
    float hum = DATA_MANAGER_Get_Humidity();

    // Send JSON (timestamp auto-read from RTC)
    sensor_json_output_send("PERIODIC", temp, hum);

    // Also log to SD card
    SDCardManager_WriteData(0, temp, hum, "PERIODIC");
}
```

**Output** (every interval):

```
{"mode":"PERIODIC""timestamp":1710000005,"temperature":25.52,"humidity":65.18}
{"mode":"PERIODIC""timestamp":1710000010,"temperature":25.54,"humidity":65.15}
{"mode":"PERIODIC""timestamp":1710000015,"temperature":25.55,"humidity":65.12}
```

### Manual Timestamp (SD Card Sync)

```c
void SyncBufferedData(void)
{
    sd_data_record_t record;
    char json[256];

    while (SDCardManager_ReadData(&record))
    {
        // Format with stored timestamp
        sensor_json_format(json, sizeof(json),
                          record.mode,
                          record.temperature,
                          record.humidity,
                          record.timestamp);

        // Send to ESP32
        PRINT_CLI("%s\r\n", json);

        // Wait for ACK, then remove
        if (WaitForACK())
        {
            SDCardManager_RemoveRecord();
        }
    }
}
```

### Error Handling

```c
char buffer[256];
int result = sensor_json_format(buffer, sizeof(buffer),
                                 "PERIODIC", temp, hum, timestamp);

if (result < 0)
{
    printf("JSON formatting failed\n");
}
else if (result >= sizeof(buffer))
{
    printf("Buffer too small: need %d bytes\n", result);
}
else
{
    printf("JSON formatted successfully: %d bytes\n", result);
    TransmitJSON(buffer);
}
```

## Integration with System Components

### Data Manager Integration

```c
// Retrieve data from Data Manager
float temp = DATA_MANAGER_Get_Temperature();
float hum = DATA_MANAGER_Get_Humidity();
uint8_t mode = DATA_MANAGER_Get_Mode();

// Convert mode to string
const char *mode_str = (mode == PERIODIC_MODE) ? "PERIODIC" : "SINGLE";

// Send JSON
sensor_json_output_send(mode_str, temp, hum);
```

### RTC Integration

```c
// Automatic RTC reading when timestamp = 0
sensor_json_output_send("PERIODIC", 25.5, 65.2);
// Internally calls DS3231_ReadTime() and DS3231_ReadDate()
```

### UART Transmission

```c
// sensor_json_output_send() uses PRINT_CLI internally:
void sensor_json_output_send(const char *mode, float temperature, float humidity)
{
    char buffer[256];
    sensor_json_format(buffer, sizeof(buffer), mode, temperature, humidity, 0);
    PRINT_CLI("%s\r\n", buffer);  // Transmit via UART
}
```

## JSON Parsing (Receiver Side)

### ESP32 JSON Parsing

On the ESP32 side, parse the JSON string:

```c
// Using cJSON library
cJSON *root = cJSON_Parse(json_string);
if (root != NULL)
{
    cJSON *timestamp = cJSON_GetObjectItem(root, "timestamp");
    cJSON *temperature = cJSON_GetObjectItem(root, "temperature");
    cJSON *humidity = cJSON_GetObjectItem(root, "humidity");
    cJSON *mode = cJSON_GetObjectItem(root, "mode");

    printf("Timestamp: %ld\n", timestamp->valueint);
    printf("Temperature: %.2f\n", temperature->valuedouble);
    printf("Humidity: %.2f\n", humidity->valuedouble);
    printf("Mode: %s\n", mode->valuestring);

    cJSON_Delete(root);
}
```

### Web Dashboard Parsing

```javascript
// JavaScript parsing
const data = JSON.parse(jsonString);
console.log("Timestamp:", data.timestamp);
console.log("Temperature:", data.temperature);
console.log("Humidity:", data.humidity);
console.log("Mode:", data.mode);

// Convert timestamp to Date
const date = new Date(data.timestamp * 1000);
console.log("Time:", date.toLocaleString());
```

## Performance Characteristics

### Execution Time

| Operation                    | Time          |
| ---------------------------- | ------------- |
| sensor_json_format()         | 100-200 µs    |
| RTC read (if timestamp = 0)  | 300-500 µs    |
| UART transmission (80 bytes) | 7 ms @ 115200 |
| **Total (send function)**    | **~8 ms**     |

### Bandwidth Usage

**JSON Size**: ~80 bytes
**With CRLF**: ~82 bytes

**Transmission Time @ 115200 baud**:

```
Time = (82 bytes × 10 bits/byte) / 115200 bits/sec
     = 820 bits / 115200 bits/sec
     = 7.1 ms
```

**Periodic Mode Bandwidth** (5-second interval):

```
Bytes per hour = (82 bytes × 3600 sec) / 5 sec = 59,040 bytes ≈ 58 KB/hour
Daily bandwidth = 58 KB × 24 hours = 1.39 MB/day
```

## Memory Footprint

### RAM Usage

- Static buffer (send function): 256 bytes
- Stack during formatting: ~100 bytes
- **Total**: ~360 bytes

### Flash Usage

- Function code: ~1 KB
- snprintf implementation: ~2-3 KB (from C library)
- **Total**: ~3-4 KB

## Best Practices

### 1. Validate Mode String

```c
const char *mode = (DATA_MANAGER_Get_Mode() == PERIODIC_MODE) ? "PERIODIC" : "SINGLE";
sensor_json_output_send(mode, temp, hum);
```

### 2. Check Buffer Size

```c
char buffer[256];
int len = sensor_json_format(buffer, sizeof(buffer), mode, temp, hum, timestamp);
if (len > 0 && len < sizeof(buffer))
{
    // Success
    ProcessJSON(buffer);
}
```

### 3. Use Appropriate Timestamp

```c
// Real-time data: auto-read RTC
sensor_json_output_send("SINGLE", temp, hum);

// Buffered data: use stored timestamp
sensor_json_format(buffer, size, mode, temp, hum, record.timestamp);
```

### 4. Add Line Ending for UART

```c
// Manual transmission with line ending
char buffer[256];
sensor_json_format(buffer, sizeof(buffer), mode, temp, hum, 0);
PRINT_CLI("%s\r\n", buffer);  // Add CRLF for proper parsing
```

## Error Handling

### Buffer Overflow Protection

```c
int len = sensor_json_format(buffer, buffer_size, mode, temp, hum, timestamp);

if (len < 0)
{
    // Formatting error
    PRINT_CLI("ERROR: JSON formatting failed\r\n");
}
else if (len >= buffer_size)
{
    // Buffer too small
    PRINT_CLI("ERROR: Buffer overflow prevented\r\n");
}
else
{
    // Success
    TransmitJSON(buffer);
}
```

### Invalid Sensor Data

```c
if (temperature < -40.0 || temperature > 125.0)
{
    PRINT_CLI("ERROR: Invalid temperature\r\n");
    return;
}

if (humidity < 0.0 || humidity > 100.0)
{
    PRINT_CLI("ERROR: Invalid humidity\r\n");
    return;
}

// Data valid, proceed with JSON output
sensor_json_output_send(mode, temperature, humidity);
```

## Dependencies

### Required

- **data_manager.h**: Sensor data access
- **ds3231.h**: RTC timestamp reading
- **print_cli.h**: UART transmission

### Used By

- **cmd_parser.c**: SINGLE and PERIODIC command handlers
- **main.c**: Periodic measurement output
- **sd_card_manager.c**: Data synchronization

## Limitations

### Maximum JSON Length

**Typical**: ~80 bytes
**Maximum** (with extreme values):

```
{"mode":"PERIODIC","timestamp":2147483647,"temperature":-40.00,"humidity":100.00}
                                                                                    ^
                                                                              ~85 bytes
```

**Buffer Recommendation**: 128 bytes minimum, 256 bytes for safety.

### Timestamp Range

Unix timestamp is 32-bit unsigned:

- **Minimum**: 0 (January 1, 1970)
- **Maximum**: 2,147,483,647 (January 19, 2038, 03:14:07 UTC)
- **Year 2038 Problem**: System will need update before 2038

### Floating Point Precision

Limited to 2 decimal places:

- Input: 25.5678
- Output: "25.57" (rounded)

For higher precision, modify format specifier in source code.

## Summary

The Sensor JSON Output Library provides:

- Standardized JSON serialization for sensor data
- Automatic RTC timestamp integration
- Compact JSON format (no whitespace)
- UART transmission via PRINT_CLI
- Buffer management options (internal or external)
- Thread-safe formatting (with external buffer)
- ~8ms total output time
- ~80 bytes per JSON message

This library enables seamless integration between STM32, ESP32, and web dashboard with a consistent JSON data format for sensor readings and system status.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
