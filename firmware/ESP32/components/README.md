# ESP32 Components Library

This directory contains reusable firmware components for the ESP32 in the DATALOGGER system. Each component provides a specific functionality and can be used independently or combined to build complete IoT applications.

## Component Files

```
components/                     # Custom component libraries
├── button_handler/             # Physical button input processing
│   ├── button_handler.c
│   ├── button_handler.h
│   ├── button_config.h
│   ├── CMakeLists.txt
│   └── Kconfig
│
├── coap_handler/               # CoAP protocol support (optional)
│   ├── coap_handler.c
│   ├── coap_handler.h
│   ├── CMakeLists.txt
│   ├── Kconfig
│   └── README.md
│
├── json_sensor_parser/         # JSON parsing for sensor data
│   ├── json_sensor_parser.c
│   ├── json_sensor_parser.h
│   ├── CMakeLists.txt
│   └── README.md
│
├── json_utils/                 # JSON utility functions
│   ├── json_utils.c
│   ├── json_utils.h
│   ├── CMakeLists.txt
│   └── README.md
│
├── mqtt_handler/               # MQTT v5 client implementation
│   ├── mqtt_handler.c
│   ├── mqtt_handler.h
│   ├── CMakeLists.txt
│   ├── Kconfig
│   └── README.md
│
├── relay_control/              # GPIO relay control
│   ├── relay_control.c
│   ├── relay_control.h
│   ├── CMakeLists.txt
│   ├── Kconfig
│   └── README.md
│
├── ring_buffer/                # Circular buffer for UART reception
│   ├── ring_buffer.c
│   ├── ring_buffer.h
│   ├── CMakeLists.txt
│   └── README.md
│
├── stm32_uart/                 # UART communication with STM32
│   ├── stm32_uart.c
│   ├── stm32_uart.h
│   ├── CMakeLists.txt
│   ├── Kconfig
│   └── README.md
│
├── wifi_manager/               # WiFi connection management
│   ├── wifi_manager.c
│   ├── wifi_manager.h
│   ├── led_config.h
│   ├── CMakeLists.txt
│   ├── Kconfig
│   └── Kconfig.projbuild
│
├── espressif__coap/            # Third-party CoAP library
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── ...
│
└── README.md                   # This file
```

## Overview

The DATALOGGER ESP32 firmware is built using a modular component architecture. Each component encapsulates a specific functionality with well-defined APIs, making the codebase maintainable, testable, and reusable across different projects.

## Component Architecture

```
┌────────────────────────────────────────────────────────────────────────────────────┐
│                             Application Layer (main.c)                             │
│  ┌─────────────────────────────────────────────────────────────────────────────┐   │
│  │    State Management (g_device_on, g_periodic_active)                        │   │
│  │    Callback Orchestration                                                   │   │
│  │    Component Lifecycle Control                                              │   │
│  └─────────────────────────────────────────────────────────────────────────────┘   │
└──┬────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬─────────┬──┘
   │        │         │         │         │         │         │         │         │
   │        │         │         │         │         │         │         │         │
   ↓        ↓         ↓         ↓         ↓         ↓         ↓         ↓         ↓
┌─────┐  ┌─────┐  ┌───────┐  ┌─────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌───────┐  ┌──────┐
│ Btn │  │Relay│  │ JSON  │  │JSON │  │ WiFi │  │ MQTT │  │ CoAP │  │ STM32 │  │ RING │
│Hand │  │Ctrl │  │ Utils │  │Parse│  │  Mgr │  │Handle│  │Handle│  │ UART  │  │BUFFER│
└──┬──┘  └──┬──┘  └───┬───┘  └──┬──┘  └──┬───┘  └───┬──┘  └───┬──┘  └───┬───┘  └──┬───┘
   │        │         │         │        │          │         │         │         │
   │        │         │         │        │          │         │         │         │
   └────────┼─────────┼─────────┼────────┼──────────┼─────────┼─────────┼─────────┘
            │         │         │        │          │         │         │
            ↓         ↓         ↓        ↓          ↓         ↓         ↓
         ┌────────────────────────────────────────────────────────────────┐
         │                 Component Interaction Layer                    │
         └────────────────────────────────────────────────────────────────┘
```

## Components List

### Communication Components

| Component                                  | Purpose                         | Key Features                                                         |
| ------------------------------------------ | ------------------------------- | -------------------------------------------------------------------- |
| **[wifi_manager](wifi_manager/README.md)** | WiFi connectivity management    | Auto-reconnect, RSSI monitoring, power save modes, IPv4/IPv6 support |
| **[mqtt_handler](mqtt_handler/README.md)** | MQTT v5.0 client wrapper        | Pub/Sub messaging, QoS 0-2, auto-reconnect, retained messages        |
| **[coap_handler](coap_handler/README.md)** | CoAP client for low-latency IoT | UDP-based, OBSERVE support, minimal overhead (~5KB)                  |
| **[stm32_uart](stm32_uart/README.md)**     | UART communication with STM32   | 115200 baud, line-based parsing, ring buffer, ISR-safe               |

### Data Processing Components

| Component                                              | Purpose                           | Key Features                                                     |
| ------------------------------------------------------ | --------------------------------- | ---------------------------------------------------------------- |
| **[json_sensor_parser](json_sensor_parser/README.md)** | Parse JSON sensor data from STM32 | Validates temp/humidity, detects sensor failures, mode detection |
| **[json_utils](json_utils/README.md)**                 | Create consistent JSON messages   | Centralized formatting, buffer safety, standardized keys         |
| **[ring_buffer](ring_buffer/README.md)**               | Circular FIFO buffer for UART     | 256-byte capacity, ISR-safe, O(1) operations                     |

### Control Components

| Component                                      | Purpose                        | Key Features                                                      |
| ---------------------------------------------- | ------------------------------ | ----------------------------------------------------------------- |
| **[relay_control](relay_control/README.md)**   | GPIO relay control             | ON/OFF control, command parsing, state callbacks, toggle function |
| **[button_handler](button_handler/README.md)** | Physical button input handling | Debouncing (50ms), multiple buttons (4 max), active-low detection |

## Quick Start

### 1. Basic WiFi Connection

```c
#include "wifi_manager.h"

void app_main(void) {
    // Initialize WiFi with default config from menuconfig
    wifi_manager_init(NULL);
    wifi_manager_connect();

    // Wait for connection (30 second timeout)
    if (wifi_manager_wait_connected(30000) == ESP_OK) {
        printf("WiFi connected!\n");

        char ip[16];
        wifi_manager_get_ip_addr(ip, sizeof(ip));
        printf("IP Address: %s\n", ip);
    }
}
```

### 2. MQTT Publishing

```c
#include "mqtt_handler.h"
#include "json_utils.h"

mqtt_handler_t mqtt;

void app_main(void) {
    // Connect to WiFi first
    wifi_manager_init(NULL);
    wifi_manager_connect();
    wifi_manager_wait_connected(30000);

    // Initialize MQTT
    MQTT_Handler_Init(&mqtt, "mqtt://192.168.1.100:1883",
                      "username", "password", NULL);
    MQTT_Handler_Start(&mqtt);

    // Create and publish sensor data
    char json[256];
    JSON_Utils_CreateSensorData(json, sizeof(json),
                                "SINGLE", time(NULL), 25.5, 60.0);

    MQTT_Handler_Publish(&mqtt, "datalogger/sensor/data",
                        json, 0, 0, 0);
}
```

### 3. STM32 Communication

```c
#include "stm32_uart.h"
#include "json_sensor_parser.h"

stm32_uart_t uart;
json_sensor_parser_t parser;

void on_sensor_data(const sensor_data_t *data) {
    printf("Temperature: %.2f°C, Humidity: %.2f%%\n",
           data->temperature, data->humidity);
}

void on_stm32_line(const char *line) {
    JSON_Parser_ProcessLine(&parser, line);
}

void app_main(void) {
    // Initialize JSON parser
    JSON_Parser_Init(&parser, on_sensor_data, NULL, NULL);

    // Initialize UART to STM32
    STM32_UART_Init(&uart, UART_NUM_2, 115200, 17, 16, on_stm32_line);
    STM32_UART_StartTask(&uart);

    // Send command to STM32
    STM32_UART_SendCommand(&uart, "SHT3X SINGLE HIGH");
}
```

### 4. Complete System Integration

```c
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "stm32_uart.h"
#include "json_sensor_parser.h"
#include "json_utils.h"
#include "relay_control.h"
#include "button_handler.h"

mqtt_handler_t mqtt;
stm32_uart_t stm32;
json_sensor_parser_t parser;
relay_control_t relay;

// Handle sensor data from STM32
void on_sensor_data(const sensor_data_t *data) {
    if (!data->valid) return;

    // Create JSON message
    char json[256];
    JSON_Utils_CreateSensorData(json, sizeof(json),
                                data->mode == MODE_SINGLE ? "SINGLE" : "PERIODIC",
                                data->timestamp,
                                data->temperature,
                                data->humidity);

    // Publish to MQTT
    MQTT_Handler_Publish(&mqtt, "datalogger/sensor/data", json, 0, 0, 0);
}

// Handle MQTT commands
void on_mqtt_message(const char *topic, const char *data, int len) {
    if (strcmp(topic, "datalogger/relay") == 0) {
        char cmd[32];
        snprintf(cmd, sizeof(cmd), "%.*s", len, data);
        Relay_ProcessCommand(&relay, cmd);
    }
}

// Handle button press
void on_button_press(gpio_num_t gpio) {
    printf("Button pressed - toggling relay\n");
    Relay_Toggle(&relay);
}

void on_stm32_line(const char *line) {
    JSON_Parser_ProcessLine(&parser, line);
}

void app_main(void) {
    // 1. Initialize WiFi
    wifi_manager_init(NULL);
    wifi_manager_connect();
    wifi_manager_wait_connected(30000);

    // 2. Initialize MQTT
    MQTT_Handler_Init(&mqtt, "mqtt://192.168.1.100:1883",
                      "DataLogger", "password", on_mqtt_message);
    MQTT_Handler_Start(&mqtt);
    MQTT_Handler_Subscribe(&mqtt, "datalogger/relay", 1);

    // 3. Initialize JSON parser
    JSON_Parser_Init(&parser, on_sensor_data, on_sensor_data, NULL);

    // 4. Initialize STM32 UART
    STM32_UART_Init(&stm32, UART_NUM_2, 115200, 17, 16, on_stm32_line);
    STM32_UART_StartTask(&stm32);

    // 5. Initialize relay control
    Relay_Init(&relay, GPIO_NUM_4, NULL);

    // 6. Initialize button handler
    Button_Init(GPIO_NUM_0, on_button_press);
    Button_StartTask();

    printf("DATALOGGER system initialized!\n");
}
```

## Configuration

All components support configuration via ESP-IDF menuconfig:

```bash
idf.py menuconfig
```

Navigate to **Component config** to find component-specific settings:

- **WiFi Connection Configuration** - SSID, password, retry, scan settings
- **MQTT Handler Configuration** - Broker URL, credentials, timeouts
- **CoAP Handler Configuration** - Server IP, port, retry settings
- **STM32 UART Configuration** - Baud rate, GPIO pins, buffer sizes
- **Relay Control Configuration** - Default GPIO pin, initial state
- **Button Handler Configuration** - Debounce time, max buttons, polling interval

## Component Dependencies

```
button_handler
  └─ ESP-IDF GPIO driver

coap_handler
  ├─ ESP-IDF libcoap
  └─ lwIP UDP stack

json_sensor_parser
  └─ C standard library

json_utils
  └─ C standard library

mqtt_handler
  ├─ ESP-IDF MQTT client
  └─ ESP-IDF networking

relay_control
  └─ ESP-IDF GPIO driver

ring_buffer
  └─ C standard library

stm32_uart
  ├─ ESP-IDF UART driver
  ├─ ring_buffer component
  └─ FreeRTOS

wifi_manager
  ├─ ESP-IDF WiFi driver
  ├─ ESP-IDF event loop
  └─ ESP-IDF network interface
```

## Protocol Comparison

### MQTT vs CoAP

| Feature        | MQTT                                    | CoAP                         |
| -------------- | --------------------------------------- | ---------------------------- |
| **Transport**  | TCP (reliable)                          | UDP (best-effort)            |
| **Connection** | Persistent                              | Connectionless               |
| **Overhead**   | Medium (~15-20KB)                       | Low (~5KB)                   |
| **Latency**    | 50-200ms                                | 10-50ms                      |
| **Use Case**   | Reliable messaging, broker architecture | Real-time sensors, low power |
| **Port**       | 1883 (TCP)                              | 5683 (UDP)                   |

**Recommendation:**

- Use **MQTT** for: Commands, configuration, guaranteed delivery
- Use **CoAP** for: Frequent sensor readings, real-time data, battery-powered devices

## Memory Usage

Typical memory footprint per component (approximate):

| Component          | RAM Usage          | Flash Usage |
| ------------------ | ------------------ | ----------- |
| wifi_manager       | ~2 KB              | ~15 KB      |
| mqtt_handler       | ~4 KB              | ~25 KB      |
| coap_handler       | ~2 KB              | ~20 KB      |
| stm32_uart         | ~512 B             | ~8 KB       |
| json_sensor_parser | ~512 B             | ~5 KB       |
| json_utils         | ~0 B (stack only)  | ~3 KB       |
| relay_control      | ~24 B              | ~4 KB       |
| button_handler     | ~128 B (4 buttons) | ~6 KB       |
| ring_buffer        | ~260 B             | ~2 KB       |

**Total (all components):** ~9.5 KB RAM, ~88 KB Flash

## Performance Characteristics

### Communication Latency

- **WiFi Connection**: 2-5 seconds (fast scan)
- **MQTT Publish (QoS 0)**: 1-10ms
- **MQTT Publish (QoS 1-2)**: 10-50ms
- **CoAP Publish**: 10-50ms
- **UART Transmission**: <1ms per message

### Processing Speed

- **JSON Parsing**: <1ms per message
- **JSON Creation**: <100µs per message
- **Button Debouncing**: 50ms (configurable)
- **Relay State Change**: <100µs

## Building and Flashing

### Build System

This component library uses ESP-IDF build system (CMake):

```bash
# Configure project
idf.py menuconfig

# Build firmware
idf.py build

# Flash to ESP32
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py monitor
```

### Adding Components to Your Project

Create `CMakeLists.txt` in your project:

```cmake
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Add component paths
set(EXTRA_COMPONENT_DIRS "path/to/components")

project(datalogger)
```

Include components in your `main/CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES
        wifi_manager
        mqtt_handler
        stm32_uart
        json_sensor_parser
        json_utils
        relay_control
        button_handler
        ring_buffer
        coap_handler
)
```

## Testing

### Unit Testing

Each component includes inline documentation and usage examples. For integration testing:

```c
// Test WiFi connection
void test_wifi(void) {
    wifi_manager_init(NULL);
    wifi_manager_connect();
    assert(wifi_manager_wait_connected(30000) == ESP_OK);
    printf("✓ WiFi test passed\n");
}

// Test MQTT publish
void test_mqtt(void) {
    MQTT_Handler_Init(&mqtt, "mqtt://broker", "user", "pass", NULL);
    MQTT_Handler_Start(&mqtt);

    char json[256];
    JSON_Utils_CreateSensorData(json, sizeof(json), "SINGLE", 0, 25.0, 60.0);

    int msg_id = MQTT_Handler_Publish(&mqtt, "test/topic", json, 0, 0, 0);
    assert(msg_id > 0);
    printf("✓ MQTT test passed\n");
}
```

### Debugging

Enable debug logging:

```bash
idf.py menuconfig
# Component config → Log output → Default log verbosity → Debug
```

Monitor specific components:

```bash
# Monitor all WiFi messages
idf.py monitor | grep "wifi"

# Monitor MQTT activity
idf.py monitor | grep "MQTT"

# Monitor UART communication
idf.py monitor | grep "STM32_UART"
```

## Common Issues and Solutions

### WiFi Connection Fails

**Problem:** WiFi doesn't connect  
**Solutions:**

- Verify SSID/password in menuconfig
- Check signal strength (RSSI)
- Increase connection timeout
- Try fast scan method

### MQTT Publish Fails

**Problem:** Messages not reaching broker  
**Solutions:**

- Ensure WiFi is connected first
- Verify broker URL and credentials
- Check firewall/network settings
- Monitor MQTT debug logs

### UART Data Loss

**Problem:** Missing data from STM32  
**Solutions:**

- Check TX/RX pin connections
- Verify baud rate (115200)
- Ensure common ground connection
- Monitor ring buffer overflow

### Relay Not Responding

**Problem:** Relay doesn't change state  
**Solutions:**

- Verify GPIO18 connection
- Check relay module power supply
- Test with direct GPIO control
- Check command format

## License

This component library is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
