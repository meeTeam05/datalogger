# MQTT Handler Component

## Overview

MQTT client wrapper for ESP32 with support for MQTT v5.0 protocol. This component provides a simple interface for connecting ESP32 to MQTT brokers, publishing sensor data, and subscribing to command topics using the modern MQTT v5.0 standard.

## Features

- MQTT 5.0 Protocol Support: modern protocol with enhanced metadata and error handling
- Auto-Reconnect: automatic reconnection on network loss
- QoS Levels 0-2: configurable quality of service
- Topic Subscription Management: easy subscribe/unsubscribe
- Callback System: custom handler for incoming messages
- Retained Messages: persist messages on the broker

## Configuration

- Broker URL: e.g., `mqtt://broker.example.com`
- Authentication: optional username/password support
- Event Callbacks: handle messages asynchronously
- Simple API: easy to integrate into ESP-IDF projects

## Usage

### Example

```c
#include "mqtt_handler.h"

// Callback function for incoming data
void on_data(const char *topic, const char *data, int len) {
    printf("%s: %.*s\n", topic, len, data);
}

void app_main(void) {
    mqtt_handler_t mqtt;

    // 1. Initialize MQTT handler with broker settings
    MQTT_Handler_Init(&mqtt, "mqtt://broker:1883", "user", "pass", on_data);

    // 2. Connect to MQTT broker
    MQTT_Handler_Start(&mqtt);

    // 3. Subscribe to desired topics
    MQTT_Handler_Subscribe(&mqtt, "device/command", 1);

    // 4. Publish/receive messages as needed
    MQTT_Handler_Publish(&mqtt, "device/data", "25.5", 0, 0, 0);
}
```

---

## Key Functions

| Function                     | Description                                            |
| ---------------------------- | ------------------------------------------------------ |
| `MQTT_Handler_Init()`        | Initialize MQTT client with broker URL and credentials |
| `MQTT_Handler_Start()`       | Connect to the MQTT broker                             |
| `MQTT_Handler_Subscribe()`   | Subscribe to a topic with specific QoS                 |
| `MQTT_Handler_Publish()`     | Publish a message to a topic                           |
| `MQTT_Handler_IsConnected()` | Check MQTT connection status                           |

---

## License

MIT License - see project root for details.
