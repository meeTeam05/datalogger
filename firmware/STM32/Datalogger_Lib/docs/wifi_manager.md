# MQTT Manager Library

Tracks MQTT connection state for the STM32 Datalogger system.

## Overview

The MQTT Manager Library provides a simple interface to check the current MQTT connection state. The actual state is managed externally (typically in `main.c`) and this library only provides read access to that state through a getter function.

**Note**: Despite the filename `wifi_manager`, this library tracks MQTT connection state, not WiFi connectivity. The ESP32 handles WiFi management independently.

## Files

- **wifi_manager.h**: MQTT state API and type definitions
- **wifi_manager.c**: MQTT state getter implementation

## MQTT Connection States

### State Enumeration
```c
typedef enum
{
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_CONNECTED
} mqtt_state_t;
```

**State Descriptions**:

| State                     | Value | Meaning                            |
| ------------------------- | ----- | ---------------------------------- |
| `MQTT_STATE_DISCONNECTED` | 0     | ESP32 not connected to MQTT broker |
| `MQTT_STATE_CONNECTED`    | 1     | ESP32 connected to MQTT broker     |

## API Functions

### Get MQTT Connection State
```c
mqtt_state_t mqtt_manager_get_state(void);
```

Returns the current MQTT connection state.

**Returns**:
- `MQTT_STATE_DISCONNECTED`: MQTT broker unreachable or ESP32 offline
- `MQTT_STATE_CONNECTED`: MQTT broker connected, ready for data transmission

**Implementation**:
```c
mqtt_state_t mqtt_manager_get_state(void)
{
    return mqtt_current_state;
}
```

**External Variable Required**:
```c
// Must be declared in main.c or another module
extern mqtt_state_t mqtt_current_state;
```

## Usage Examples

### Example 1: Conditional Data Transmission
```c
#include "wifi_manager.h"
#include "sensor_json_output.h"

void SendSensorData(float temp, float hum)
{
    if (mqtt_manager_get_state() == MQTT_STATE_CONNECTED)
    {
        // MQTT available - send to cloud
        sensor_json_output_send(0, temp, hum, "SINGLE");
        printf("Data sent to MQTT broker\r\n");
    }
    else
    {
        // MQTT unavailable - cannot send
        printf("MQTT disconnected, data not transmitted\r\n");
    }
}
```

### Example 2: Check Before Sending
```c
#include "wifi_manager.h"

void TransmitData(void)
{
    // Always check MQTT state before transmission
    if (mqtt_manager_get_state() == MQTT_STATE_CONNECTED)
    {
        UART_Transmit("DATA:123\r\n");
    }
}
```

### Example 3: Status Display on LCD
```c
#include "wifi_manager.h"
#include "display.h"

void UpdateMQTTStatusDisplay(void)
{
    mqtt_state_t state = mqtt_manager_get_state();
    
    if (state == MQTT_STATE_CONNECTED)
    {
        DISPLAY_DrawString(10, 130, "MQTT", FONT_11X18, COLOR_WHITE);
        DISPLAY_DrawCircle(80, 138, 3, COLOR_GREEN, 1);  // Green dot
    }
    else
    {
        DISPLAY_DrawString(10, 130, "MQTT", FONT_11X18, COLOR_WHITE);
        DISPLAY_DrawCircle(80, 138, 3, COLOR_RED, 1);    // Red dot
    }
}
```

## State Management

### External State Variable

The library reads from an external variable that must be defined elsewhere:
```c
// In main.c
mqtt_state_t mqtt_current_state = MQTT_STATE_DISCONNECTED;
```

**Default State**: Should be initialized to `MQTT_STATE_DISCONNECTED` (safe default).

### Updating State

State updates are handled **outside** this library. Typical implementation in `main.c`:
```c
// Update when ESP32 sends status via UART
void UpdateMQTTState(const char* status)
{
    if (strcmp(status, "MQTT_CONNECTED") == 0)
    {
        mqtt_current_state = MQTT_STATE_CONNECTED;
        printf("MQTT broker connected\r\n");
    }
    else if (strcmp(status, "MQTT_DISCONNECTED") == 0)
    {
        mqtt_current_state = MQTT_STATE_DISCONNECTED;
        printf("MQTT broker disconnected\r\n");
    }
}
```

## Integration Example

### Complete Integration in main.c
```c
#include "wifi_manager.h"
#include "uart.h"
#include "sht3x.h"
#include "sensor_json_output.h"

// Define the state variable
mqtt_state_t mqtt_current_state = MQTT_STATE_DISCONNECTED;

int main(void)
{
    // Hardware initialization
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    SHT3X_Init(&g_sht3x, &hi2c1, 0x44);
    UART_Init(&huart1);
    
    while (1)
    {
        // Process UART commands from ESP32
        if (UART_DataAvailable())
        {
            char cmd[64];
            UART_ReadLine(cmd, sizeof(cmd));
            
            // Check for MQTT status updates
            if (strstr(cmd, "MQTT_CONNECTED"))
            {
                mqtt_current_state = MQTT_STATE_CONNECTED;
            }
            else if (strstr(cmd, "MQTT_DISCONNECTED"))
            {
                mqtt_current_state = MQTT_STATE_DISCONNECTED;
            }
        }
        
        // Periodic sensor measurement
        if (Flag_Periodic)
        {
            Flag_Periodic = 0;
            
            float temp, hum;
            SHT3X_ReadData(&g_sht3x, &temp, &hum);
            
            // Check MQTT state before sending
            if (mqtt_manager_get_state() == MQTT_STATE_CONNECTED)
            {
                sensor_json_output_send(0, temp, hum, "PERIODIC");
            }
        }
    }
}
```

## Performance Characteristics

### Function Overhead

**mqtt_manager_get_state()**:
- Returns external variable
- Execution time: ~10 clock cycles (~0.1 µs @ 72 MHz)
- **Negligible overhead**, safe to call frequently

## Memory Footprint

### RAM Usage
- No internal state (reads external variable)
- **Total**: 0 bytes (state stored externally)

### Flash Usage
- Function code: ~20 bytes
- **Extremely lightweight library**

## Dependencies

### Required
- **mqtt_state_t**: Type definition in `wifi_manager.h`
- **mqtt_current_state**: External variable (must be defined by user)

### Used By
- **main.c**: Main loop checks state for conditional data transmission
- **sensor_json_output.c**: May check state before sending JSON
- **display.c**: May check state for status display

## Best Practices

### 1. Always Check State Before Transmission
```c
// GOOD: Check before sending
if (mqtt_manager_get_state() == MQTT_STATE_CONNECTED)
{
    UART_Transmit(data);
}

// BAD: Send without checking (may fail silently)
UART_Transmit(data);
```

### 2. Initialize State to Disconnected
```c
// Safe default prevents sending to unavailable broker
mqtt_state_t mqtt_current_state = MQTT_STATE_DISCONNECTED;
```

### 3. Update State Based on ESP32 Feedback
```c
// Listen for status messages from ESP32
void ProcessESP32Status(const char* msg)
{
    if (strstr(msg, "CONNECTED"))
    {
        mqtt_current_state = MQTT_STATE_CONNECTED;
    }
    else if (strstr(msg, "DISCONNECTED"))
    {
        mqtt_current_state = MQTT_STATE_DISCONNECTED;
    }
}
```

## Limitations

### 1. Read-Only Interface

This library only provides read access. State updates must be done directly on the external variable:
```c
// Not provided by library - must do manually
mqtt_current_state = MQTT_STATE_CONNECTED;
```

### 2. No Setter Function

There is no `mqtt_manager_set_state()` function. Update the external variable directly:
```c
// Direct access required
extern mqtt_state_t mqtt_current_state;
mqtt_current_state = MQTT_STATE_CONNECTED;
```

### 3. No State Change Callbacks

The library does not notify when state changes. Application must implement its own change detection:
```c
static mqtt_state_t previous_state = MQTT_STATE_DISCONNECTED;

void CheckStateChange(void)
{
    mqtt_state_t current_state = mqtt_manager_get_state();
    
    if (current_state != previous_state)
    {
        printf("MQTT state changed\r\n");
        previous_state = current_state;
    }
}
```

### 4. No Automatic Reconnection

This library only tracks state. Actual MQTT connection management is handled by the ESP32.

## Communication Architecture
```
┌───────────┐         UART          ┌───────────┐         WiFi/MQTT          ┌─────────────┐
│   STM32   │ ←──────────────────→  │   ESP32   │ ←────────────────────────→ │ MQTT Broker │
│ (Sensors) │       Commands        │  (WiFi)   │       Pub/Sub on Topics    │   (Cloud)   │
└───────────┘                       └───────────┘                            └─────────────┘
      │                                      │
      │                                      │
mqtt_manager_get_state()           mqtt_client_is_connected()
      │                                      │
      └──────> mqtt_current_state <──────────┘
               (external variable)
```

**Responsibilities**:
- **STM32**: Read sensors, check MQTT state via this library
- **ESP32**: Manage WiFi, connect to MQTT broker, send status to STM32
- **main.c**: Update `mqtt_current_state` based on ESP32 feedback

## Summary

The MQTT Manager Library provides:

- Simple read-only interface to MQTT connection state
- Single getter function with minimal overhead (~0.1 µs)
- No internal state - reads from external variable
- Lightweight implementation (~20 bytes flash)
- Enables conditional data transmission based on MQTT availability

**Key Point**: This is a **passive monitoring library**. It does not manage state internally - it only provides read access to an externally-managed state variable.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.