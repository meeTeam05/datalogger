# JSON Utils Component

## Overview

Universal JSON utility library for ESP32. Provides centralized JSON creation functions to ensure consistent formatting across all MQTT/CoAP messages in the datalogger system.

## Features

- Centralized Creation: Single source of truth for JSON formatting
- Buffer Safety: Automatic size checking and truncation
- Format Consistency: Standardized JSON structure
- Purpose-Built: Optimized for sensor data and system state
- Lightweight: Minimal memory footprint

## Architecture

```
Application Layer
       │
       ├─► JSON_Utils_CreateSensorData()    → MQTT Publish
       ├─► JSON_Utils_CreateSystemState()   → MQTT Publish
       ├─► JSON_Utils_CreateSimpleMessage() → MQTT Publish
       └─► JSON_Utils_CreateIntValue()      → MQTT Publish
```

## Why JSON Utils?

**Before (Inconsistent):**
```c
// File 1
sprintf(buf, "{\"temp\":%.2f,\"hum\":%.2f}", temp, hum);

// File 2
sprintf(buf, "{\"temperature\":%f,\"humidity\":%f}", temp, hum);  // Different keys!

// File 3
sprintf(buf, "{\"t\":%.1f,\"h\":%.1f}", temp, hum);  // Different precision!
```

**After (Consistent):**
```c
// All files use same function
JSON_Utils_CreateSensorData(buf, size, "SINGLE", timestamp, temp, hum);
// → Guaranteed consistent format
```

## Usage

### 1. Sensor Data JSON

Create JSON for sensor measurements:

```c
#include "json_utils.h"

char json_msg[256];

// Create sensor data JSON
int len = JSON_Utils_CreateSensorData(
    json_msg, sizeof(json_msg),
    "SINGLE",           // mode: "SINGLE" or "PERIODIC"
    1760729112,         // timestamp (Unix)
    25.50,              // temperature in °C
    60.00               // humidity in %
);

// Result:
// {"mode":"SINGLE","timestamp":1760729112,"temperature":25.50,"humidity":60.00}
```

### 2. System State JSON

Create JSON for device/periodic state:

```c
char state_msg[256];

// Create system state JSON
int len = JSON_Utils_CreateSystemState(
    state_msg, sizeof(state_msg),
    true,               // device ON
    false,              // periodic OFF
    0                   // timestamp (deprecated, use 0)
);

// Result:
// {"device":"ON","periodic":"OFF"}
```

### 3. Simple Key-Value JSON

Create JSON with single key-value pair:

```c
char simple_msg[128];

// Create simple message
int len = JSON_Utils_CreateSimpleMessage(
    simple_msg, sizeof(simple_msg),
    "status",           // key
    "OK"                // value
);

// Result:
// {"status":"OK"}
```

### 4. Integer Value JSON

Create JSON with integer value:

```c
char int_msg[128];

// Create integer value JSON
int len = JSON_Utils_CreateIntValue(
    int_msg, sizeof(int_msg),
    "interval",         // key
    5                   // value (seconds)
);

// Result:
// {"interval":5}
```

## API Reference

### Sensor Data
```c
int JSON_Utils_CreateSensorData(
    char *buffer,           // Output buffer
    size_t buffer_size,     // Buffer size in bytes
    const char *mode,       // "SINGLE" or "PERIODIC"
    uint32_t timestamp,     // Unix timestamp
    float temperature,      // Temperature in °C
    float humidity          // Humidity in %
);
```

**Returns**: Number of characters written, or -1 on error

**Example output**:
```json
{"mode":"SINGLE","timestamp":1760729112,"temperature":25.50,"humidity":60.00}
```

### System State
```c
int JSON_Utils_CreateSystemState(
    char *buffer,           // Output buffer
    size_t buffer_size,     // Buffer size in bytes
    bool device_on,         // Device relay state
    bool periodic_active,   // Periodic mode active
    uint64_t timestamp      // Deprecated (use 0)
);
```

**Returns**: Number of characters written, or -1 on error

**Example output**:
```json
{"device":"ON","periodic":"OFF"}
```

**Note**: `timestamp` parameter is deprecated and ignored. Pass 0 for forward compatibility.

### Simple Message
```c
int JSON_Utils_CreateSimpleMessage(
    char *buffer,           // Output buffer
    size_t buffer_size,     // Buffer size in bytes
    const char *key,        // JSON key name
    const char *value       // String value
);
```

**Returns**: Number of characters written, or -1 on error

**Example output**:
```json
{"status":"connected"}
```

### Integer Value
```c
int JSON_Utils_CreateIntValue(
    char *buffer,           // Output buffer
    size_t buffer_size,     // Buffer size in bytes
    const char *key,        // JSON key name
    int value               // Integer value
);
```

**Returns**: Number of characters written, or -1 on error

**Example output**:
```json
{"count":42}
```

## Format Standards

### Sensor Data Format
- **Temperature**: 2 decimal places (e.g., `25.50`)
- **Humidity**: 2 decimal places (e.g., `60.00`)
- **Timestamp**: Integer (Unix timestamp)
- **Mode**: String ("SINGLE" or "PERIODIC")

### System State Format
- **Device**: String ("ON" or "OFF")
- **Periodic**: String ("ON" or "OFF")
- **No timestamp**: Removed for simplicity

### Key Naming Convention
- Use descriptive names: `temperature` not `temp`
- Lowercase only: `device` not `Device`
- No abbreviations unless standard: `humidity` not `hum`

## Buffer Safety

All functions check buffer size before writing:

```c
char small_buffer[10];  // Too small

int len = JSON_Utils_CreateSensorData(small_buffer, sizeof(small_buffer),
                                      "SINGLE", 1760729112, 25.50, 60.00);

if (len < 0) {
    printf("ERROR: Buffer too small!\n");  // Safe error handling
}
```

**Recommended buffer sizes:**
- Sensor data: 256 bytes
- System state: 128 bytes
- Simple message: 128 bytes
- Integer value: 64 bytes

## Error Handling

Functions return `-1` on error:

```c
int len = JSON_Utils_CreateSensorData(NULL, 0, "SINGLE", 0, 0.0, 0.0);
if (len < 0) {
    // Handle error
    printf("JSON creation failed\n");
}
```

**Error conditions:**
- `buffer` is NULL
- `buffer_size` is 0
- Required string parameter (mode, key, value) is NULL
- Generated JSON exceeds buffer size

## Performance

- **Creation time**: < 100µs per message
- **Memory**: Stack-allocated buffers only
- **CPU overhead**: Minimal (sprintf-based)
- **Deterministic**: Fixed execution time

## Integration Example

```c
#include "json_utils.h"
#include "mqtt_handler.h"

void publish_sensor_data(float temp, float hum) {
    char json_msg[256];
    
    // Create JSON using utils
    int len = JSON_Utils_CreateSensorData(
        json_msg, sizeof(json_msg),
        "SINGLE",
        time(NULL),  // Current timestamp
        temp,
        hum
    );
    
    if (len > 0) {
        // Publish via MQTT
        MQTT_Handler_Publish(&mqtt, 
                           "datalogger/sensor/data",
                           json_msg, 
                           0, 0, 0);
    }
}
```

## Testing

```c
void test_json_utils(void) {
    char buffer[256];
    int len;
    
    // Test sensor data
    len = JSON_Utils_CreateSensorData(buffer, sizeof(buffer),
                                      "SINGLE", 1234567890,
                                      25.50, 60.00);
    assert(len > 0);
    assert(strstr(buffer, "\"mode\":\"SINGLE\"") != NULL);
    assert(strstr(buffer, "\"temperature\":25.50") != NULL);
    
    // Test system state
    len = JSON_Utils_CreateSystemState(buffer, sizeof(buffer),
                                       true, false, 0);
    assert(len > 0);
    assert(strstr(buffer, "\"device\":\"ON\"") != NULL);
    assert(strstr(buffer, "\"periodic\":\"OFF\"") != NULL);
    
    // Test buffer overflow
    char small[10];
    len = JSON_Utils_CreateSensorData(small, sizeof(small),
                                      "SINGLE", 0, 0.0, 0.0);
    assert(len < 0);  // Should fail
    
    printf("All tests passed!\n");
}
```

## Benefits

**Consistency**: All JSON messages use same format  
**Maintainability**: Update format in one place  
**Type Safety**: Function parameters enforce correct types  
**Buffer Safety**: Automatic overflow prevention  
**Readability**: Clear function names and parameters  

## Migration Guide

**Old code:**
```c
sprintf(buf, "{\"mode\":\"%s\",\"timestamp\":%u,\"temperature\":%.2f,\"humidity\":%.2f}",
        mode, timestamp, temp, hum);
```

**New code:**
```c
JSON_Utils_CreateSensorData(buf, sizeof(buf), mode, timestamp, temp, hum);
```

## Dependencies

- C standard library (stdio.h, string.h)
- ESP-IDF types (stdint.h, stdbool.h)

## License

MIT License - see project root for details.

## Related Components

- [JSON Sensor Parser](../json_sensor_parser/README.md) - Parses JSON messages
- [MQTT Handler](../mqtt_handler/README.md) - Publishes JSON to broker
