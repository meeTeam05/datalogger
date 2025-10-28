# Relay Control Component

This component provides GPIO-based relay control functionality for the ESP32 in the DATALOGGER system. It manages relay state, processes control commands, and provides notifications of state changes through callbacks.

## Component Files

```
relay_control/
├── relay_control.h        # Public API header with structure and function declarations
├── relay_control.c        # Implementation of relay control logic
├── CMakeLists.txt         # ESP-IDF build configuration
├── component.mk           # Legacy build system support
├── Kconfig                # Configuration menu options
└── README.md              # This file
```

## Overview

The Relay Control component provides a simple interface for controlling relay modules connected to ESP32 GPIO pins. It supports direct state control, command string parsing, state toggling, and optional callback notifications when relay state changes. The default GPIO pin is GPIO18, configurable via menuconfig.

## Key Features

- GPIO pin configuration for relay control output
- Boolean state management (ON/OFF or true/false)
- Command string parsing supporting multiple formats
- State toggle function for easy switching
- Optional callback notification on state changes
- Safe initialization and deinitialization
- State query function

## Data Structure

```c
typedef struct {
    int gpio_num;                       // GPIO pin number for relay
    bool state;                         // Current relay state (true=ON, false=OFF)
    bool initialized;                   // Initialization status flag
    relay_state_callback_t state_callback; // Optional state change callback
} relay_control_t;
```

## API Functions

### Initialization

**Relay_Init**

```c
bool Relay_Init(relay_control_t *relay,
               int gpio_num,
               relay_state_callback_t callback);
```

Initializes GPIO pin for relay control and configures initial state.

Parameters:

- relay: Pointer to relay control structure
- gpio_num: GPIO pin number to use for relay control (typically GPIO18)
- callback: Optional function to call on state changes (can be NULL)

Returns:

- true: Initialization successful, GPIO configured as output
- false: Initialization failed (invalid GPIO or already initialized)

The function configures the specified GPIO as output, sets initial state to OFF (low level), and registers the optional callback.

### State Control

**Relay_SetState**

```c
bool Relay_SetState(relay_control_t *relay, bool state);
```

Sets the relay to a specific state.

Parameters:

- relay: Pointer to initialized relay control structure
- state: Desired relay state (true=ON/high, false=OFF/low)

Returns:

- true: State changed successfully
- false: Operation failed (relay not initialized)

If a callback is registered, it will be invoked after the state change.

**Relay_GetState**

```c
bool Relay_GetState(relay_control_t *relay);
```

Retrieves the current relay state.

Parameters:

- relay: Pointer to relay control structure

Returns:

- true: Relay is ON
- false: Relay is OFF or not initialized

**Relay_Toggle**

```c
bool Relay_Toggle(relay_control_t *relay);
```

Toggles the relay state (ON to OFF or OFF to ON).

Parameters:

- relay: Pointer to relay control structure

Returns:

- true: New state after toggle (ON)
- false: New state after toggle (OFF) or toggle failed

If a callback is registered, it will be invoked after toggling.

### Command Processing

**Relay_ProcessCommand**

```c
bool Relay_ProcessCommand(relay_control_t *relay, const char *command);
```

Parses a command string and sets relay state accordingly.

Parameters:

- relay: Pointer to relay control structure
- command: Command string to process

Returns:

- true: Command recognized and executed
- false: Command not recognized or execution failed

Supported command formats (case-insensitive):

- "ON", "on", "On": Turn relay ON
- "OFF", "off", "Off": Turn relay OFF
- "1": Turn relay ON
- "0": Turn relay OFF
- "RELAY ON": Turn relay ON
- "RELAY OFF": Turn relay OFF

### Cleanup

**Relay_Deinit**

```c
void Relay_Deinit(relay_control_t *relay);
```

Deinitializes the relay control and resets GPIO pin.

Parameters:

- relay: Pointer to relay control structure

Resets the GPIO pin to input mode and clears initialization flag.

## Usage Example

```c
#include "relay_control.h"

// Declare relay control structure
relay_control_t relay;

// Optional callback for state changes
void on_relay_state_change(bool state) {
    printf("Relay is now %s\n", state ? "ON" : "OFF");

    // Publish state to MQTT
    const char *state_msg = state ? "ON" : "OFF";
    MQTT_Handler_Publish(&mqtt, "datalogger/esp32/relay/state",
                        state_msg, 0, 0, 1);  // QoS 0, retained
}

void app_main(void) {
    // Initialize relay on GPIO18
    if (!Relay_Init(&relay, 18, on_relay_state_change)) {
        printf("Relay initialization failed!\n");
        return;
    }

    printf("Relay initialized on GPIO18\n");

    // Turn relay ON
    Relay_SetState(&relay, true);

    vTaskDelay(pdMS_TO_TICKS(5000));

    // Turn relay OFF
    Relay_SetState(&relay, false);

    vTaskDelay(pdMS_TO_TICKS(5000));

    // Toggle relay state
    bool new_state = Relay_Toggle(&relay);
    printf("Relay toggled to %s\n", new_state ? "ON" : "OFF");

    // Process command string
    Relay_ProcessCommand(&relay, "RELAY ON");

    // Query current state
    bool current_state = Relay_GetState(&relay);
    printf("Current relay state: %s\n", current_state ? "ON" : "OFF");
}
```

## Integration with MQTT

Typical integration in DATALOGGER system:

```c
// MQTT callback processes relay commands
void on_mqtt_message(const char *topic, const char *data, int data_len) {
    if (strcmp(topic, "datalogger/esp32/relay") == 0) {
        // Process relay command
        char command[32];
        snprintf(command, sizeof(command), "%.*s", data_len, data);

        if (Relay_ProcessCommand(&relay, command)) {
            printf("Relay command executed: %s\n", command);
        } else {
            printf("Invalid relay command: %s\n", command);
        }
    }
}
```

## Hardware Connection

Connect relay module to ESP32:

```
ESP32 GPIO18 -----> Relay Module Signal Pin
ESP32 GND  <-----> Relay Module GND
External 3.3V -----> Relay Module VCC (if required)
```

Notes:

- Most relay modules require 3.3V power supply
- GPIO18 provides 3.3V logic signal (sufficient for most modules)
- Use optocoupler-based relay modules for electrical isolation
- For higher power loads, use appropriate relay ratings

## Configuration via Menuconfig

Configure relay parameters in ESP-IDF menuconfig:

```
Component config → Relay Control Configuration
```

Available options:

- Default GPIO Pin (default: 18)
- Initial State (default: OFF)
- Enable Debug Logging (default: disabled)

## Command String Formats

The component recognizes multiple command string formats for flexibility:

**Standard Format**

```
"RELAY ON"   -> Turn ON
"RELAY OFF"  -> Turn OFF
```

**Simple Format**

```
"ON"   -> Turn ON
"OFF"  -> Turn OFF
```

**Numeric Format**

```
"1"  -> Turn ON
"0"  -> Turn OFF
```

All formats are case-insensitive. Extra whitespace is trimmed automatically.

## State Change Notifications

When a callback is registered, it is invoked after every state change:

```c
void my_callback(bool new_state) {
    // Called whenever relay state changes
    // new_state: true=ON, false=OFF

    // Update UI, log change, publish to MQTT, etc.
}
```

Callback is invoked from the same context that called the state change function.

## Error Handling

The component handles various error conditions:

**Initialization Failures**

- GPIO pin already in use by another component
- Invalid GPIO pin number
- Memory allocation failure

**Operation Failures**

- Relay not initialized before use
- Invalid command string format
- GPIO write error

All failures return false from respective functions. Check return values to detect errors.

## GPIO Pin Selection

Recommended GPIO pins for relay control on ESP32:

- GPIO4: Safe, no special boot requirements
- GPIO5: Safe, no special boot requirements
- GPIO18: Safe, no special boot requirements
- GPIO19: Safe, no special boot requirements

Avoid these GPIO pins:

- GPIO0: Boot mode selection (must be HIGH at boot)
- GPIO2: Boot mode selection (must be floating/HIGH at boot)
- GPIO12: Flash voltage selection (boot fails if pulled HIGH)
- GPIO15: Boot mode selection (must be LOW at boot)

## Performance Characteristics

- Initialization Time: Less than 1 ms
- State Change Time: Less than 100 microseconds
- Command Processing: Less than 1 ms
- Memory Usage: 24 bytes per relay instance
- CPU Overhead: Negligible (direct GPIO operations)

## Limitations

- Single relay per structure instance (create multiple instances for multiple relays)
- No hardware PWM support (on/off only, no dimming)
- No advanced features like pulse or timed operation
- Callback executes in caller's context (keep processing brief)

## Extending Functionality

To add timed relay operation:

```c
// Turn on for specific duration
void Relay_SetStateForDuration(relay_control_t *relay,
                               bool state,
                               uint32_t duration_ms) {
    Relay_SetState(relay, state);

    // Create timer to turn off after duration
    // (Implementation depends on requirements)
}
```

To add pulse generation:

```c
// Generate pulse on relay output
void Relay_Pulse(relay_control_t *relay, uint32_t pulse_ms) {
    Relay_SetState(relay, true);
    vTaskDelay(pdMS_TO_TICKS(pulse_ms));
    Relay_SetState(relay, false);
}
```

## Debugging

Enable debug logging for relay operations:

```c
// In sdkconfig or via menuconfig
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y

// In relay_control.c
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
```

Monitor relay activity:

```bash
idf.py monitor | grep "RELAY"
```

## Dependencies

- ESP-IDF GPIO driver (driver/gpio.h)
- ESP-IDF logging (esp_log.h)
- C standard library (string.h for command parsing)

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
