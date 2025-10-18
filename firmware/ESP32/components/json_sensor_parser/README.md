# JSON Sensor Parser Component

## Overview

Universal JSON sensor data parser for ESP32. Parses JSON-formatted sensor data from STM32, validates measurements, and triggers callbacks for different operating modes.

## Features

- JSON Parsing: Efficient parsing of sensor data messages
- Data Validation: Range checking and CRC verification
- Mode Detection: Automatic SINGLE/PERIODIC mode recognition
- Callback System: Event-driven architecture for data handling
- Error Handling: Robust error detection and reporting
- Extensible: Easy to add new sensor types

## Architecture

```
STM32 → UART → JSON Parser → Callbacks → MQTT/CoAP
                    │
                    ├─► SINGLE mode callback
                    ├─► PERIODIC mode callback
                    └─► ERROR callback
```

## Data Format

### Input JSON Format

```json
{
  "mode": "SINGLE",
  "timestamp": 1760729112,
  "temperature": 25.50,
  "humidity": 60.00
}
```

### Fields

| Field | Type | Description | Range |
|-------|------|-------------|-------|
| mode | string | Operating mode | "SINGLE" or "PERIODIC" |
| timestamp | uint32 | Unix timestamp from RTC | 0+ (0 = RTC failure) |
| temperature | float | Temperature in Celsius | -40 to 125°C (0.00 = sensor fail) |
| humidity | float | Relative humidity | 0 to 100% (0.00 = sensor fail) |

## Usage

### 1. Initialize Parser

```c
#include "json_sensor_parser.h"

json_sensor_parser_t parser;

// Define callbacks
void on_single_data(const sensor_data_t *data) {
    printf("SINGLE: T=%.2f°C, H=%.2f%%\n", 
           data->temperature, data->humidity);
}

void on_periodic_data(const sensor_data_t *data) {
    printf("PERIODIC: T=%.2f°C, H=%.2f%%\n",
           data->temperature, data->humidity);
}

void on_error(const sensor_data_t *data) {
    printf("Parse error!\n");
}

// Initialize parser
JSON_Parser_Init(&parser, 
                 on_single_data,
                 on_periodic_data,
                 on_error);
```

### 2. Process Incoming Data

```c
// When JSON line received from STM32
const char *json_line = "{\"mode\":\"SINGLE\",\"timestamp\":1760729112,\"temperature\":25.50,\"humidity\":60.00}";

// Parse and trigger callback
JSON_Parser_ProcessLine(&parser, json_line);
```

### 3. Manual Parsing

```c
// Parse without callbacks
sensor_data_t data = JSON_Parser_ParseLine(&parser, json_line);

if (data.valid) {
    printf("Temperature: %.2f°C\n", data.temperature);
    printf("Humidity: %.2f%%\n", data.humidity);
}
```

## API Reference

### Initialization
```c
bool JSON_Parser_Init(json_sensor_parser_t *parser,
                      sensor_data_callback_t single_callback,
                      sensor_data_callback_t periodic_callback,
                      sensor_data_callback_t error_callback);
```

### Processing
```c
// Parse and trigger callbacks
bool JSON_Parser_ProcessLine(json_sensor_parser_t *parser, 
                              const char *json_line);

// Parse only (no callbacks)
sensor_data_t JSON_Parser_ParseLine(json_sensor_parser_t *parser,
                                     const char *json_line);
```

### Utilities
```c
// Get mode from string
sensor_mode_t JSON_Parser_GetMode(const char *mode_str);

// Get mode string
const char *JSON_Parser_GetModeString(sensor_mode_t mode);

// Validate sensor data
bool JSON_Parser_IsValid(const sensor_data_t *data);

// Check for hardware failures
bool JSON_Parser_IsSensorFailed(const sensor_data_t *data);
bool JSON_Parser_IsRTCFailed(const sensor_data_t *data);
```

## Data Structure

```c
typedef struct {
    // Metadata
    sensor_mode_t mode;      // SINGLE or PERIODIC
    uint32_t timestamp;      // Unix timestamp
    bool valid;              // Overall validity flag
    
    // SHT3X sensor data
    bool has_temperature;    // Temperature field present
    float temperature;       // Temperature in °C
    bool has_humidity;       // Humidity field present
    float humidity;          // Humidity in %
    
    // Future extensibility
    // bool has_pressure;
    // float pressure;
} sensor_data_t;
```

## Validation Rules

### Temperature
- **Valid range**: -40°C to 125°C
- **Special value**: 0.00 indicates sensor hardware failure
- **Out of range**: Data marked as invalid

### Humidity
- **Valid range**: 0% to 100%
- **Special value**: 0.00 indicates sensor hardware failure
- **Out of range**: Data marked as invalid

### Timestamp
- **Valid**: Any positive Unix timestamp
- **Special value**: 0 indicates RTC failure
- **Both timestamp=0 AND T=H=0.00**: Critical hardware failure

## Error Handling

```c
sensor_data_t data = JSON_Parser_ParseLine(&parser, json_line);

if (!data.valid) {
    printf("Parsing failed!\n");
    return;
}

// Check for sensor failure
if (JSON_Parser_IsSensorFailed(&data)) {
    printf("WARNING: Sensor hardware failure detected\n");
}

// Check for RTC failure
if (JSON_Parser_IsRTCFailed(&data)) {
    printf("WARNING: RTC not configured\n");
}
```

## Performance

- **Parse time**: < 1ms per message
- **Memory usage**: ~512 bytes parser state
- **CPU overhead**: Minimal (string parsing only)
- **Thread-safe**: No (single-threaded use)

## Extensibility

To add new sensor types (e.g., pressure, CO2):

1. Update `sensor_data_t` structure:
```c
typedef struct {
    // ... existing fields ...
    
    // New sensor field
    bool has_pressure;
    float pressure;  // Pressure in hPa
} sensor_data_t;
```

2. Update parsing logic in `JSON_Parser_ParseLine()`:
```c
// Extract pressure (optional)
float pressure;
if (json_get_float(json_line, "pressure", &pressure)) {
    data.has_pressure = true;
    data.pressure = pressure;
}
```

3. Update JSON format:
```json
{
  "mode": "SINGLE",
  "timestamp": 1760729112,
  "temperature": 25.50,
  "humidity": 60.00,
  "pressure": 1013.25
}
```

## Dependencies

- ESP-IDF (esp_log)
- C standard library (string.h, stdio.h, stdlib.h)

## Testing

```c
// Test valid JSON
const char *test1 = "{\"mode\":\"SINGLE\",\"timestamp\":1760729112,\"temperature\":25.50,\"humidity\":60.00}";
sensor_data_t data1 = JSON_Parser_ParseLine(&parser, test1);
assert(data1.valid == true);

// Test invalid JSON
const char *test2 = "{invalid json}";
sensor_data_t data2 = JSON_Parser_ParseLine(&parser, test2);
assert(data2.valid == false);

// Test sensor failure
const char *test3 = "{\"mode\":\"SINGLE\",\"timestamp\":1760729112,\"temperature\":0.00,\"humidity\":0.00}";
sensor_data_t data3 = JSON_Parser_ParseLine(&parser, test3);
assert(JSON_Parser_IsSensorFailed(&data3) == true);
```

## Troubleshooting

### Parser returns invalid data
- Check JSON format (must start with `{` and end with `}`)
- Verify all required fields present: `mode`, `timestamp`
- Check field names match exactly (case-sensitive)

### Callbacks not triggered
- Ensure `JSON_Parser_Init()` called with valid callbacks
- Use `JSON_Parser_ProcessLine()` not `JSON_Parser_ParseLine()`
- Check parser initialization succeeded

### Wrong mode detected
- Mode string must be exactly "SINGLE" or "PERIODIC"
- Case-sensitive matching

## License

MIT License - see project root for details.

## Related Components

- [STM32 UART](../stm32_uart/README.md) - Receives JSON from STM32
- [MQTT Handler](../mqtt_handler/README.md) - Publishes parsed data
- [JSON Utils](../json_utils/README.md) - Creates JSON messages
