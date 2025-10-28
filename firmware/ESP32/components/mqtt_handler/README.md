# MQTT Handler Component

This component provides an MQTT v5.0 client wrapper for ESP32, enabling simplified communication with MQTT brokers for publishing sensor data and receiving control commands in the DATALOGGER system.

## Component Files

```
mqtt_handler/
├── mqtt_handler.h         # Public API header with structure and function declarations
├── mqtt_handler.c         # Implementation of MQTT client wrapper
├── CMakeLists.txt         # ESP-IDF build configuration
├── component.mk           # Legacy build system support
├── Kconfig                # Configuration menu options
└── README.md              # This file
```

## Overview

The MQTT Handler component wraps the ESP-IDF MQTT client library to provide a simpler interface for MQTT v5.0 protocol operations. It handles connection management, automatic reconnection with exponential backoff, topic subscription, message publishing, and asynchronous message reception through callbacks.

## Key Features

- MQTT v5.0 protocol support with modern features and enhanced error handling
- Automatic client ID generation from ESP32 MAC address
- Automatic reconnection with exponential backoff on connection loss
- QoS levels 0, 1, and 2 support for reliable message delivery
- Topic subscription with configurable quality of service
- Message publishing with retain flag support
- Asynchronous message reception via callback mechanism
- Connection status monitoring
- Configurable broker URL, username, and password authentication

## Data Structure

```c
typedef struct {
    esp_mqtt_client_handle_t client;    // ESP-IDF MQTT client handle
    mqtt_data_callback_t data_callback; // Callback for incoming messages
    bool connected;                     // Current connection status
    char client_id[32];                 // Unique client identifier
    uint32_t retry_count;               // Reconnection attempt counter
    uint32_t last_retry_time_ms;        // Timestamp of last retry
} mqtt_handler_t;
```

## API Functions

### Initialization

**MQTT_Handler_Init**

```c
bool MQTT_Handler_Init(mqtt_handler_t *mqtt,
                       const char *broker_url,
                       const char *username,
                       const char *password,
                       mqtt_data_callback_t callback);
```

Initializes the MQTT handler with broker connection parameters and registers a callback for incoming messages.

Parameters:

- mqtt: Pointer to MQTT handler structure
- broker_url: MQTT broker URL (e.g., "mqtt://192.168.1.100:1883")
- username: Authentication username (NULL for anonymous)
- password: Authentication password (NULL for anonymous)
- callback: Function to call when messages are received

Returns:

- true: Initialization successful
- false: Initialization failed (check broker URL format and memory availability)

The function automatically generates a unique client ID from the ESP32 MAC address to prevent broker client ID conflicts.

### Connection Management

**MQTT_Handler_Start**

```c
bool MQTT_Handler_Start(mqtt_handler_t *mqtt);
```

Starts the MQTT client and initiates connection to the broker.

Parameters:

- mqtt: Pointer to initialized MQTT handler structure

Returns:

- true: Connection process started successfully
- false: Start failed (MQTT not initialized)

The actual connection is asynchronous. Use MQTT_Handler_IsConnected() to check connection status.

**MQTT_Handler_IsConnected**

```c
bool MQTT_Handler_IsConnected(mqtt_handler_t *mqtt);
```

Checks the current MQTT connection status.

Parameters:

- mqtt: Pointer to MQTT handler structure

Returns:

- true: Client is connected to broker
- false: Client is disconnected

**MQTT_Handler_Reconnect**

```c
bool MQTT_Handler_Reconnect(mqtt_handler_t *mqtt);
```

Attempts to reconnect to the MQTT broker with exponential backoff.

Parameters:

- mqtt: Pointer to MQTT handler structure

Returns:

- true: Reconnection attempt initiated
- false: Backoff delay not elapsed, retry later

This function implements exponential backoff to prevent overwhelming the broker with rapid reconnection attempts.

### Topic Subscription

**MQTT_Handler_Subscribe**

```c
int MQTT_Handler_Subscribe(mqtt_handler_t *mqtt,
                          const char *topic,
                          int qos);
```

Subscribes to an MQTT topic to receive messages.

Parameters:

- mqtt: Pointer to MQTT handler structure
- topic: Topic string to subscribe (supports wildcards # and +)
- qos: Quality of Service level (0, 1, or 2)

Returns:

- Positive message ID: Subscription request sent successfully
- -1: Subscription failed (not connected or invalid parameters)

QoS levels:

- 0: At most once (fire and forget)
- 1: At least once (acknowledged delivery)
- 2: Exactly once (guaranteed single delivery)

### Message Publishing

**MQTT_Handler_Publish**

```c
int MQTT_Handler_Publish(mqtt_handler_t *mqtt,
                        const char *topic,
                        const char *data,
                        int data_len,
                        int qos,
                        int retain);
```

Publishes a message to an MQTT topic.

Parameters:

- mqtt: Pointer to MQTT handler structure
- topic: Topic string to publish to
- data: Message payload data
- data_len: Length of data in bytes (0 for null-terminated string)
- qos: Quality of Service level (0, 1, or 2)
- retain: Retain flag (0 or 1)

Returns:

- Positive message ID: Publish request sent successfully
- -1: Publish failed (not connected or invalid parameters)

When retain is set to 1, the broker stores the message and delivers it to new subscribers immediately upon subscription.

### Cleanup

**MQTT_Handler_Stop**

```c
void MQTT_Handler_Stop(mqtt_handler_t *mqtt);
```

Stops the MQTT client and disconnects from broker.

Parameters:

- mqtt: Pointer to MQTT handler structure

**MQTT_Handler_Deinit**

```c
void MQTT_Handler_Deinit(mqtt_handler_t *mqtt);
```

Deinitializes the MQTT handler and frees allocated resources.

Parameters:

- mqtt: Pointer to MQTT handler structure

## Usage Example

```c
#include "mqtt_handler.h"

// Declare MQTT handler
mqtt_handler_t mqtt;

// Callback function for received messages
void on_mqtt_message(const char *topic, const char *data, int data_len) {
    printf("Received on %s: %.*s\n", topic, data_len, data);

    // Handle different topics
    if (strcmp(topic, "datalogger/esp32/command") == 0) {
        // Process command
        handle_command(data, data_len);
    }
}

void app_main(void) {
    // Initialize MQTT handler
    bool init_ok = MQTT_Handler_Init(&mqtt,
                                    "mqtt://192.168.1.100:1883",
                                    "DataLogger",
                                    "password",
                                    on_mqtt_message);

    if (!init_ok) {
        printf("MQTT initialization failed!\n");
        return;
    }

    // Start MQTT client
    if (!MQTT_Handler_Start(&mqtt)) {
        printf("MQTT start failed!\n");
        return;
    }

    // Wait for connection
    while (!MQTT_Handler_IsConnected(&mqtt)) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    printf("MQTT connected!\n");

    // Subscribe to command topic
    MQTT_Handler_Subscribe(&mqtt, "datalogger/esp32/command", 1);
    MQTT_Handler_Subscribe(&mqtt, "datalogger/esp32/relay", 1);

    // Publish sensor data
    char json_data[] = "{\"temperature\":25.5,\"humidity\":60.0}";
    MQTT_Handler_Publish(&mqtt,
                        "datalogger/stm32/data/json",
                        json_data,
                        0,  // Auto-detect length
                        0,  // QoS 0
                        0); // No retain
}
```

## Configuration via Menuconfig

Configure component parameters in ESP-IDF menuconfig:

```
Component config → MQTT Handler Configuration
```

Available options:

- Broker URL (default: mqtt://192.168.1.100:1883)
- MQTT Username (default: DataLogger)
- MQTT Password (default: datalogger)
- Keep Alive Interval (default: 120 seconds)
- Network Timeout (default: 10 seconds)
- Maximum Reconnect Delay (default: 60 seconds)

## Connection Flow

1. MQTT_Handler_Init() configures client parameters and generates client ID
2. MQTT_Handler_Start() initiates connection attempt
3. ESP-IDF MQTT library establishes TCP connection to broker
4. MQTT CONNECT packet sent with authentication credentials
5. Broker responds with CONNACK packet
6. Connection status updated to connected
7. Application can now subscribe and publish

## Reconnection Mechanism

The component implements automatic reconnection with exponential backoff:

1. Connection loss detected by ESP-IDF MQTT library
2. Component sets connected status to false
3. Application calls MQTT_Handler_Reconnect() periodically
4. Backoff delay calculated: min(2^retry_count seconds, max_delay)
5. If delay elapsed, reconnection attempt initiated
6. On successful connection, retry count reset to zero
7. On failure, retry count incremented for longer next delay

Initial delay: 1 second
Maximum delay: 60 seconds (configurable)

## Message Reception

When a message is received on a subscribed topic:

1. ESP-IDF MQTT library receives PUBLISH packet
2. Internal event handler extracts topic and payload
3. Registered callback function invoked with topic, data, and length
4. Application processes message in callback context
5. For QoS 1 and 2, acknowledgment sent automatically

Important: Callback executes in MQTT event task context. Keep processing time short or defer work to another task.

## QoS Level Behavior

**QoS 0 (At most once)**

- No acknowledgment required
- Message may be lost if network fails
- Lowest overhead, highest throughput
- Suitable for frequent sensor readings where occasional loss acceptable

**QoS 1 (At least once)**

- Broker acknowledges with PUBACK
- Message guaranteed delivered but may duplicate
- Moderate overhead
- Suitable for important commands that can handle duplicates

**QoS 2 (Exactly once)**

- Four-way handshake ensures single delivery
- Highest overhead, lowest throughput
- Suitable for critical operations requiring no loss or duplication

## Topic Wildcards

MQTT supports two wildcard characters in subscriptions:

**Single-level wildcard (+)**

```c
MQTT_Handler_Subscribe(&mqtt, "datalogger/+/data", 1);
// Matches: datalogger/esp32/data, datalogger/stm32/data
// Doesn't match: datalogger/esp32/data/temperature
```

**Multi-level wildcard (#)**

```c
MQTT_Handler_Subscribe(&mqtt, "datalogger/esp32/#", 1);
// Matches all topics starting with datalogger/esp32/
```

Note: Wildcards only allowed in subscriptions, not in publish topics.

## Error Handling

**Connection Failures**

- Check broker URL format (must start with mqtt:// or mqtts://)
- Verify broker IP address and port reachable
- Confirm authentication credentials if required
- Monitor ESP32 WiFi connection status

**Publish Failures**

- Verify connection status before publishing
- Check topic string validity (no wildcards, not empty)
- Ensure data length does not exceed broker limits
- Monitor for message queue overflow

**Subscription Failures**

- Confirm connection established before subscribing
- Verify topic string format
- Check QoS level is valid (0, 1, or 2)
- Ensure broker allows subscriptions to requested topics

## Performance Characteristics

- Client ID Generation: One-time at initialization
- Connection Establishment: 100-500 ms typical
- Publish Latency: 1-10 ms for QoS 0, higher for QoS 1-2
- Memory Usage: Approximately 4 KB per handler instance
- CPU Utilization: Less than 1% at typical message rates

## Integration with Other Components

This component integrates with:

**JSON Utils Component**: Uses JSON_Utils_CreateSensorData() to format messages for publishing

**JSON Sensor Parser Component**: Parses received JSON command messages

**STM32 UART Component**: Publishes sensor data received from STM32

**Relay Control Component**: Subscribes to relay control commands

## Debugging

Enable MQTT logging for detailed information:

```c
// In sdkconfig or via menuconfig
CONFIG_MQTT_PROTOCOL_311=n
CONFIG_MQTT_PROTOCOL_5=y
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
```

Monitor MQTT activity:

```bash
idf.py monitor | grep "MQTT"
```

Common log messages:

- "MQTT_EVENT_CONNECTED": Successful connection
- "MQTT_EVENT_DISCONNECTED": Connection lost
- "MQTT_EVENT_SUBSCRIBED": Subscription confirmed
- "MQTT_EVENT_PUBLISHED": Message published
- "MQTT_EVENT_DATA": Message received

## Dependencies

- ESP-IDF MQTT client library (mqtt_client.h)
- ESP-IDF networking components (WiFi)
- FreeRTOS (for event handling)
- ESP-IDF logging (esp_log.h)

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
