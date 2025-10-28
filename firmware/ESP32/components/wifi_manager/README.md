# WiFi Manager Component

This component provides comprehensive WiFi management functionality for the ESP32 in the DATALOGGER system. It handles WiFi connection, disconnection, state monitoring, power management, and network information retrieval with automatic retry and event notification capabilities.

## Component Files

```
wifi_manager/
├── wifi_manager.h        # Public API header with structures and function declarations
├── wifi_manager.c        # Implementation of WiFi management logic
├── led_config.h          # LED indicator configuration for WiFi status
├── CMakeLists.txt        # ESP-IDF build configuration
├── Kconfig.projbuild     # Configuration menu options for WiFi settings
└── README.md             # This file
```

## Overview

The WiFi Manager component provides a high-level abstraction for ESP32 WiFi connectivity. It manages the entire WiFi lifecycle including initialization, connection with automatic retry, disconnection, state monitoring, signal strength measurement, IP address retrieval, and power management. The component supports both IPv4 and IPv6, multiple security modes, and provides flexible callback mechanisms for event notification.

## Key Features

- Automatic WiFi connection with configurable retry mechanism
- State machine with four states: DISCONNECTED, CONNECTING, CONNECTED, FAILED
- Event-driven callback system for state change notifications
- Signal strength (RSSI) monitoring
- IP address retrieval (IPv4 and optional IPv6)
- Power save mode support for battery-powered applications
- Configurable AP scanning (fast scan or all-channel scan)
- AP sorting by RSSI or security level
- RSSI threshold filtering for reliable connections
- Security mode threshold filtering (open, WEP, WPA, WPA2, WPA3)
- Configurable listen interval for power optimization
- Connection timeout with blocking wait function
- Thread-safe state access
- Comprehensive menuconfig integration

## State Machine

```
                    ┌─────────────────┐
                    │  DISCONNECTED   │
                    └────────┬────────┘
                             │
                             │ wifi_manager_connect()
                             ↓
                    ┌─────────────────┐
           ┌───────►│   CONNECTING    │
           │        └────────┬────────┘
           │                 │
           │                 ├─────────► Connection Success
           │                 │                    │
           │                 │                    ↓
           │                 │           ┌─────────────────┐
           │                 │           │    CONNECTED    │
           │                 │           └────────┬────────┘
           │                 │                    │
           │                 │                    │ wifi_manager_disconnect()
           │                 │                    │
           │                 ├─────────► Connection Failure
           │                 │                    │
           │                 │                    ↓
           │                 │           ┌─────────────────┐
           │                 │           │     FAILED      │
           │                 │           └────────┬────────┘
           │                 │                    │
           │                 │                    │ Retry
           │                 │                    │
           └─────────────────┴────────────────────┘
                             │
                             ↓
                    ┌─────────────────┐
                    │  DISCONNECTED   │
                    └─────────────────┘
```

## Configuration Structure

```c
typedef struct {
    const char *ssid;                     // WiFi SSID (network name)
    const char *password;                 // WiFi password (NULL for open networks)
    uint8_t maximum_retry;                // Maximum connection retry attempts
    uint8_t scan_method;                  // 0=fast scan, 1=all channel scan
    uint8_t sort_method;                  // 0=sort by RSSI, 1=sort by security
    int8_t rssi_threshold;                // Minimum RSSI (dBm) for connection
    wifi_auth_mode_t auth_mode_threshold; // Minimum security mode
    uint16_t listen_interval;             // Listen interval for power save
    bool power_save_enabled;              // Enable power save mode
    wifi_ps_type_t power_save_mode;       // Power save mode type
    bool ipv6_enabled;                    // Enable IPv6 support
    uint32_t connection_timeout_ms;       // Connection timeout in milliseconds
    wifi_event_callback_t event_callback; // State change callback function
    void *callback_arg;                   // User argument for callback
} wifi_manager_config_t;
```

## WiFi States

```c
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,    // Not connected to any network
    WIFI_STATE_CONNECTING,          // Attempting to connect
    WIFI_STATE_CONNECTED,           // Successfully connected
    WIFI_STATE_FAILED               // Connection attempt failed
} wifi_state_t;
```

## API Functions

### Configuration

**wifi_manager_get_default_config**

```c
wifi_manager_config_t wifi_manager_get_default_config(void);
```

Retrieves default WiFi configuration from Kconfig settings.

Returns:

- Configuration structure populated with values from menuconfig

This function loads SSID, password, retry count, scan method, sort method, RSSI threshold, security threshold, listen interval, power save settings, IPv6 settings, and connection timeout from Kconfig definitions. Use this as a starting point and modify specific fields as needed.

### Initialization

**wifi_manager_init**

```c
esp_err_t wifi_manager_init(wifi_manager_config_t *config);
```

Initializes WiFi manager with the provided configuration.

Parameters:

- config: Pointer to configuration structure (pass NULL to use default Kconfig settings)

Returns:

- ESP_OK: Initialization successful
- ESP_ERR_INVALID_ARG: Invalid configuration
- ESP_ERR_NO_MEM: Memory allocation failed
- ESP_FAIL: WiFi initialization failed

This function initializes the WiFi subsystem, registers event handlers, configures WiFi parameters, and prepares for connection. It must be called before any other WiFi manager functions.

### Connection Management

**wifi_manager_connect**

```c
esp_err_t wifi_manager_connect(void);
```

Initiates WiFi connection to the configured network.

Returns:

- ESP_OK: Connection process started
- ESP_ERR_INVALID_STATE: WiFi manager not initialized
- ESP_FAIL: Failed to start connection

This function is non-blocking. It starts the connection process and returns immediately. Use wifi_manager_wait_connected() to block until connected or wifi_manager_is_connected() to poll connection status.

**wifi_manager_disconnect**

```c
esp_err_t wifi_manager_disconnect(void);
```

Disconnects from the current WiFi network.

Returns:

- ESP_OK: Disconnection successful
- ESP_ERR_INVALID_STATE: WiFi manager not initialized
- ESP_FAIL: Disconnection failed

Stops WiFi connection and transitions to DISCONNECTED state.

**wifi_manager_wait_connected**

```c
esp_err_t wifi_manager_wait_connected(uint32_t timeout_ms);
```

Blocks until WiFi connection is established or timeout expires.

Parameters:

- timeout_ms: Maximum wait time in milliseconds (0 = wait forever)

Returns:

- ESP_OK: Successfully connected
- ESP_ERR_TIMEOUT: Connection timeout
- ESP_ERR_INVALID_STATE: WiFi manager not initialized

Useful for ensuring WiFi is connected before proceeding with network operations.

### State Monitoring

**wifi_manager_get_state**

```c
wifi_state_t wifi_manager_get_state(void);
```

Retrieves the current WiFi connection state.

Returns:

- Current state: DISCONNECTED, CONNECTING, CONNECTED, or FAILED

Thread-safe function for polling WiFi state.

**wifi_manager_is_connected**

```c
bool wifi_manager_is_connected(void);
```

Checks if WiFi is currently connected.

Returns:

- true: WiFi is connected
- false: WiFi is not connected

Convenience function equivalent to checking if state equals WIFI_STATE_CONNECTED.

### Network Information

**wifi_manager_get_rssi**

```c
esp_err_t wifi_manager_get_rssi(int8_t *rssi);
```

Retrieves the Received Signal Strength Indicator (RSSI) of the connected network.

Parameters:

- rssi: Pointer to store RSSI value in dBm

Returns:

- ESP_OK: RSSI retrieved successfully
- ESP_ERR_INVALID_ARG: NULL pointer
- ESP_ERR_INVALID_STATE: Not connected
- ESP_FAIL: Failed to get RSSI

RSSI values typically range from -100 dBm (very weak) to -30 dBm (excellent).

**wifi_manager_get_ip_addr**

```c
esp_err_t wifi_manager_get_ip_addr(char *ip_addr, size_t len);
```

Retrieves the current IP address assigned to the WiFi interface.

Parameters:

- ip_addr: Buffer to store IP address string (minimum 16 bytes for IPv4)
- len: Buffer length

Returns:

- ESP_OK: IP address retrieved successfully
- ESP_ERR_INVALID_ARG: NULL pointer or insufficient buffer
- ESP_ERR_INVALID_STATE: Not connected
- ESP_FAIL: Failed to get IP address

The IP address is formatted as a null-terminated string (e.g., "192.168.1.100").

### Cleanup

**wifi_manager_deinit**

```c
esp_err_t wifi_manager_deinit(void);
```

Deinitializes WiFi manager and releases resources.

Returns:

- ESP_OK: Deinitialization successful
- ESP_FAIL: Deinitialization failed

Stops WiFi, unregisters event handlers, and frees allocated memory.

## Usage Example

### Basic Connection

```c
#include "wifi_manager.h"

void app_main(void) {
    // Initialize with default config from menuconfig
    esp_err_t ret = wifi_manager_init(NULL);
    if (ret != ESP_OK) {
        printf("WiFi manager initialization failed: %d\n", ret);
        return;
    }

    // Connect to WiFi
    ret = wifi_manager_connect();
    if (ret != ESP_OK) {
        printf("WiFi connection start failed: %d\n", ret);
        return;
    }

    // Wait for connection with 30 second timeout
    ret = wifi_manager_wait_connected(30000);
    if (ret == ESP_OK) {
        printf("WiFi connected!\n");

        // Get IP address
        char ip[16];
        if (wifi_manager_get_ip_addr(ip, sizeof(ip)) == ESP_OK) {
            printf("IP Address: %s\n", ip);
        }

        // Get signal strength
        int8_t rssi;
        if (wifi_manager_get_rssi(&rssi) == ESP_OK) {
            printf("Signal Strength: %d dBm\n", rssi);
        }
    } else {
        printf("WiFi connection timeout\n");
    }
}
```

### Custom Configuration with Callback

```c
#include "wifi_manager.h"

// WiFi event callback
void on_wifi_event(wifi_state_t state, void *arg) {
    switch (state) {
        case WIFI_STATE_CONNECTING:
            printf("Connecting to WiFi...\n");
            break;

        case WIFI_STATE_CONNECTED:
            printf("WiFi connected successfully!\n");

            // Start MQTT, CoAP, or other network services
            break;

        case WIFI_STATE_DISCONNECTED:
            printf("WiFi disconnected\n");

            // Stop network services
            break;

        case WIFI_STATE_FAILED:
            printf("WiFi connection failed\n");
            break;
    }
}

void app_main(void) {
    // Start with default config
    wifi_manager_config_t config = wifi_manager_get_default_config();

    // Customize configuration
    config.maximum_retry = 10;              // Retry up to 10 times
    config.rssi_threshold = -70;            // Only connect if RSSI >= -70 dBm
    config.auth_mode_threshold = WIFI_AUTH_WPA2_PSK;  // Require WPA2 or better
    config.power_save_enabled = true;       // Enable power save mode
    config.power_save_mode = WIFI_PS_MIN_MODEM;  // Minimum modem power save
    config.event_callback = on_wifi_event;  // Register callback
    config.callback_arg = NULL;

    // Initialize with custom config
    esp_err_t ret = wifi_manager_init(&config);
    if (ret != ESP_OK) {
        printf("WiFi manager initialization failed\n");
        return;
    }

    // Connect (callback will notify state changes)
    wifi_manager_connect();
}
```

### Periodic Signal Monitoring

```c
void monitor_wifi_signal(void *pvParameters) {
    while (1) {
        if (wifi_manager_is_connected()) {
            int8_t rssi;
            if (wifi_manager_get_rssi(&rssi) == ESP_OK) {
                printf("WiFi RSSI: %d dBm\n", rssi);

                // Take action on weak signal
                if (rssi < -80) {
                    printf("Weak WiFi signal detected!\n");
                    // Potentially switch to fallback network
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10000));  // Check every 10 seconds
    }
}

void app_main(void) {
    wifi_manager_init(NULL);
    wifi_manager_connect();
    wifi_manager_wait_connected(30000);

    // Create monitoring task
    xTaskCreate(monitor_wifi_signal, "wifi_monitor", 2048, NULL, 5, NULL);
}
```

## Integration with DATALOGGER System

The WiFi manager integrates with other DATALOGGER components:

```c
void app_main(void) {
    // Initialize WiFi first
    wifi_manager_init(NULL);
    wifi_manager_connect();

    if (wifi_manager_wait_connected(30000) == ESP_OK) {
        printf("WiFi connected\n");

        // Initialize MQTT (requires WiFi)
        mqtt_handler_config_t mqtt_cfg = {
            .broker_uri = "mqtt://broker.example.com:1883",
            .client_id = "datalogger_esp32",
            // ... other MQTT config
        };
        MQTT_Handler_Init(&mqtt_handler, &mqtt_cfg);

        // Initialize CoAP (requires WiFi)
        CoAP_Handler_Init(&coap_handler);

        // Start data collection and transmission
    } else {
        printf("WiFi connection failed, entering offline mode\n");
        // Store data locally for later transmission
    }
}
```

## Configuration via Menuconfig

Configure WiFi parameters in ESP-IDF menuconfig:

```bash
idf.py menuconfig
```

Navigate to: **Component config → WiFi Connection Configuration**

Available options:

**Basic Settings**

- WiFi SSID (default: "myssid")
- WiFi Password (default: "mypassword")
- Maximum Retry Attempts (default: 5)
- Connection Timeout (default: 30000 ms)

**Scan Configuration**

- Scan Method: Fast Scan or All Channel Scan
- Sort Method: By RSSI or By Security

**Security Settings**

- Minimum RSSI Threshold (default: -127 dBm, no filtering)
- Authentication Mode Threshold (default: Open, no filtering)

**Power Management**

- Enable Power Save Mode (default: disabled)
- Power Save Type: None, Minimum Modem, or Maximum Modem
- Listen Interval (default: 3)

**Advanced Settings**

- Enable IPv6 Support (default: disabled)

## WiFi Scan Methods

**Fast Scan (scan_method = 0)**

- Scans channels sequentially
- Stops at first AP matching SSID
- Faster connection time
- May not find best AP

**All Channel Scan (scan_method = 1)**

- Scans all channels
- Finds all APs with matching SSID
- Selects best AP based on sort method
- Longer connection time but better selection

## AP Sorting Methods

**By RSSI (sort_method = 0)**

- Selects AP with strongest signal
- Best for performance and reliability
- Recommended for most applications

**By Security (sort_method = 1)**

- Selects AP with highest security level
- Priority: WPA3 > WPA2 > WPA > WEP > Open
- Best for security-critical applications

## Power Save Modes

**WIFI_PS_NONE**

- No power saving
- Maximum performance
- Highest power consumption
- Use for applications requiring low latency

**WIFI_PS_MIN_MODEM**

- Minimum modem power save
- Balanced performance and power
- DTIM-based sleep
- Recommended for most battery-powered applications

**WIFI_PS_MAX_MODEM**

- Maximum modem power save
- Maximum power savings
- May increase latency
- Use for applications tolerating higher latency

## RSSI Threshold Filtering

RSSI (Received Signal Strength Indicator) values:

- -30 dBm: Excellent signal
- -50 dBm: Very good signal
- -60 dBm: Good signal
- -70 dBm: Fair signal
- -80 dBm: Weak signal
- -90 dBm: Very weak signal

Recommended thresholds:

- Reliable operation: -70 dBm
- Acceptable operation: -80 dBm
- Minimum operation: -90 dBm

## Security Mode Thresholds

```c
wifi_auth_mode_t auth_mode_threshold;
```

Available modes (in increasing security):

- WIFI_AUTH_OPEN: No security
- WIFI_AUTH_WEP: Weak encryption (deprecated)
- WIFI_AUTH_WPA_PSK: WPA with pre-shared key
- WIFI_AUTH_WPA2_PSK: WPA2 with pre-shared key (recommended minimum)
- WIFI_AUTH_WPA_WPA2_PSK: WPA or WPA2
- WIFI_AUTH_WPA3_PSK: WPA3 with pre-shared key (most secure)

## Event Callback System

Register a callback to receive WiFi state change notifications:

```c
void my_wifi_callback(wifi_state_t state, void *arg) {
    // Handle state changes
    // This runs in event handler context - keep processing brief
}

wifi_manager_config_t config = wifi_manager_get_default_config();
config.event_callback = my_wifi_callback;
config.callback_arg = my_user_data;  // Optional context

wifi_manager_init(&config);
```

Callback is invoked on every state transition. Avoid blocking operations in callbacks.

## Error Handling

Handle connection failures gracefully:

```c
esp_err_t ret = wifi_manager_connect();
if (ret != ESP_OK) {
    printf("Failed to start connection: %d\n", ret);
    return;
}

ret = wifi_manager_wait_connected(30000);
if (ret == ESP_ERR_TIMEOUT) {
    printf("Connection timeout - check credentials and signal\n");

    // Retry with different network or enter offline mode
    wifi_manager_disconnect();

    // Try backup network
    wifi_manager_config_t backup = wifi_manager_get_default_config();
    backup.ssid = "backup_network";
    backup.password = "backup_password";
    wifi_manager_init(&backup);
    wifi_manager_connect();
}
```

## Connection Timeout Behavior

The connection timeout controls how long wifi_manager_wait_connected() will block:

```c
// Wait forever until connected
wifi_manager_wait_connected(0);

// Wait 30 seconds
wifi_manager_wait_connected(30000);

// Wait 5 seconds
wifi_manager_wait_connected(5000);
```

The maximum_retry parameter controls how many times the WiFi subsystem will retry connection attempts before giving up.

## Performance Characteristics

- Initialization Time: 100-500 ms
- Connection Time (fast scan): 2-5 seconds
- Connection Time (all channel scan): 5-15 seconds
- State Query Time: Less than 1 microsecond
- RSSI Query Time: Less than 1 ms
- IP Address Query Time: Less than 1 ms
- Memory Usage: Approximately 2 KB (plus ESP-IDF WiFi stack overhead)

## Debugging

Enable WiFi debug logging:

```c
// In sdkconfig or via menuconfig
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
```

Monitor WiFi activity:

```bash
idf.py monitor | grep "wifi"
```

Common issues:

**Connection Fails**

- Verify SSID and password in menuconfig
- Check WiFi signal strength (use site survey tool)
- Ensure AP is in range and operational
- Check RSSI and security thresholds

**Slow Connection**

- Use fast scan method
- Reduce RSSI threshold
- Increase connection timeout

**Disconnections**

- Check signal strength (RSSI)
- Verify power supply stability
- Disable power save mode for testing
- Check AP configuration (DTIM, beacon interval)

## Dependencies

- ESP-IDF WiFi driver (esp_wifi.h)
- ESP-IDF event loop (esp_event.h)
- ESP-IDF network interface (esp_netif.h)
- ESP-IDF logging (esp_log.h)
- FreeRTOS (for synchronization primitives)

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
