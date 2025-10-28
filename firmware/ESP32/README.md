# ESP32 Firmware

This directory contains the firmware for the ESP32 microcontroller, which functions as a bridge between the STM32 data acquisition system and the MQTT broker. The firmware receives sensor data via UART from the STM32, publishes data to MQTT topics, receives commands from MQTT, and controls a relay output.

## System Architecture

The ESP32 acts as a communication gateway between the STM32 sensor node and network services:

```
┌─────────────┐          ┌─────────┐          ┌───────┐
│ MQTT Broker │ ←─WiFi─→ │  ESP32  │ ←─UART─→ │ STM32 │
└─────────────┘          └────┬────┘          └───────┘
                              │
                              ↓
                       ┌──────────────┐
                       │ Relay Control│
                       │   (GPIO18)   │
                       └──────────────┘
```

Data flows in two directions:

- Upstream: STM32 data → ESP32 UART → JSON parser → MQTT publish
- Downstream: MQTT commands → ESP32 → UART transmission → STM32 execution

## Directory Structure

```
ESP32/
├── main/
│   ├── main.c                    # Application entry point and task management
│   ├── CMakeLists.txt            # Main component build configuration
│   ├── idf_component.yml         # Component dependencies
│   └── Kconfig.projbuild         # Project-specific configuration menu
│
├── components/                   # Custom component libraries
│   ├── button_handler/           # Physical button input processing
│   │   ├── button_handler.c
│   │   ├── button_handler.h
│   │   ├── button_config.h
│   │   ├── CMakeLists.txt
│   │   └── Kconfig
│   │
│   ├── coap_handler/             # CoAP protocol support (optional)
│   │   ├── coap_handler.c
│   │   ├── coap_handler.h
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   └── README.md
│   │
│   ├── json_sensor_parser/       # JSON parsing for sensor data
│   │   ├── json_sensor_parser.c
│   │   ├── json_sensor_parser.h
│   │   ├── CMakeLists.txt
│   │   └── README.md
│   │
│   ├── json_utils/               # JSON utility functions
│   │   ├── json_utils.c
│   │   ├── json_utils.h
│   │   ├── CMakeLists.txt
│   │   └── README.md
│   │
│   ├── mqtt_handler/             # MQTT v5 client implementation
│   │   ├── mqtt_handler.c
│   │   ├── mqtt_handler.h
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   └── README.md
│   │
│   ├── relay_control/            # GPIO relay control
│   │   ├── relay_control.c
│   │   ├── relay_control.h
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   └── README.md
│   │
│   ├── ring_buffer/              # Circular buffer for UART reception
│   │   ├── ring_buffer.c
│   │   ├── ring_buffer.h
│   │   ├── CMakeLists.txt
│   │   └── README.md
│   │
│   ├── stm32_uart/               # UART communication with STM32
│   │   ├── stm32_uart.c
│   │   ├── stm32_uart.h
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   └── README.md
│   │
│   ├── wifi_manager/             # WiFi connection management
│   │   ├── wifi_manager.c
│   │   ├── wifi_manager.h
│   │   ├── led_config.h
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   └── Kconfig.projbuild
│   │
│   └── espressif__coap/          # Third-party CoAP library
│       ├── CMakeLists.txt
│       ├── idf_component.yml
│       └── ...
│
├── CMakeLists.txt                # Root build configuration
├── partitions.csv                # Flash partition table
├── sdkconfig.defaults            # Default SDK configuration
├── sdkconfig.ci                  # CI/CD configuration
└── README.md                     # This file
```

Note: Each component directory contains its own README.md with detailed component-specific documentation.

## Key Features

- MQTT v5 protocol support with QoS levels and retained messages
- WiFi connection management with automatic reconnection
- UART communication with STM32 using interrupt-driven ring buffer
- JSON parsing for SHT3X sensor data (temperature and humidity)
- GPIO relay control via MQTT commands
- FreeRTOS task-based architecture for concurrent operation
- Physical button handler for local control
- Configurable parameters via menuconfig system
- CoAP protocol support (optional)

## Component Overview

### [Ring Buffer](components/ring_buffer/)

Thread-safe circular buffer implementation for UART data reception. Provides lock-free operation using volatile pointers suitable for use in interrupt service routines. Default capacity is 256 bytes.

### [STM32 UART](components/stm32_uart/)

Manages asynchronous UART communication with the STM32 microcontroller. Implements line-based data reception using a ring buffer, noise filtering, and command transmission with proper line termination. Operates as a FreeRTOS task.

### [MQTT Handler](components/mqtt_handler/)

MQTT v5 client implementation providing connection management, topic subscription/publication, and event-driven callbacks. Auto-generates client ID from ESP32 MAC address and handles automatic reconnection on connection loss.

### [Relay Control](components/relay_control/)

GPIO-based relay switching module. Supports multiple command formats (RELAY ON/OFF, DEVICE ON/OFF) with state change notifications and safe initialization.

### [JSON Sensor Parser](components/json_sensor_parser/)

Parses JSON-formatted sensor data from STM32. Validates SHT3X temperature and humidity data, supporting both single-shot and periodic measurement modes. Temperature range: -40°C to 125°C. Humidity range: 0% to 100% RH.

### [WiFi Manager](components/wifi_manager/)

Handles WiFi connection establishment and management. Provides automatic reconnection, connection status monitoring, and LED status indication.

### [Button Handler](components/button_handler/)

Processes physical button inputs with debouncing and configurable action mapping.

### [JSON Utils](components/json_utils/)

General-purpose JSON formatting and parsing utilities.

### [CoAP Handler](components/coap_handler/)

Optional CoAP protocol support for constrained network environments.

## Development Environment Setup

### Prerequisites

- ESP-IDF version 5.0 or later
- MQTT broker with MQTT v5 support (Mosquitto recommended)
- Python 3.6 or later (for ESP-IDF tools)
- USB-to-Serial driver for ESP32 programmer

### Installation Steps

1. Install ESP-IDF following the official guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/

2. Verify installation:

```bash
idf.py --version
```

3. Navigate to the ESP32 firmware directory:

```bash
cd firmware/ESP32
```

### Project Configuration

Configure the firmware parameters using menuconfig:

```bash
idf.py menuconfig
```

#### Key Configuration Sections

**ESP32 MQTT5 Bridge Configuration**

- MQTT broker URL (default: mqtt://192.168.1.100)
- MQTT username (default: DataLogger)
- MQTT password (default: datalogger)
- UART port number (default: 2)
- UART TX pin (default: GPIO17)
- UART RX pin (default: GPIO16)
- UART baud rate (default: 115200)
- Relay GPIO pin (default: GPIO18)

**Example Connection Configuration**

- WiFi SSID
- WiFi password
- WiFi authentication mode

### Building and Flashing

Build the project:

```bash
idf.py build
```

Flash the firmware to ESP32:

```bash
idf.py -p /dev/ttyUSB0 flash
```

Monitor serial output:

```bash
idf.py -p /dev/ttyUSB0 monitor
```

Combined flash and monitor:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Note: Replace /dev/ttyUSB0 with COM3 (or appropriate COM port) on Windows.

## Hardware Configuration

### Pin Assignments

| Function      | GPIO Pin              | Description                       |
| ------------- | --------------------- | --------------------------------- |
| UART TX       | GPIO17 (default)      | Transmit data to STM32            |
| UART RX       | GPIO16 (default)      | Receive data from STM32           |
| Relay Control | GPIO18 (default)      | Relay output control signal       |
| Button Input  | GPIO0 (configurable)  | Physical button for local control |
| WiFi LED      | GPIO22 (configurable) | WiFi connection status indicator  |
| MQTT LED      | GPIO23 (configurable) | MQTT connection status indicator  |

### UART Connection to STM32

Connect the ESP32 to STM32 as follows:

- ESP32 GPIO17 (TX) → STM32 PA10 (RX)
- ESP32 GPIO16 (RX) → STM32 PA9 (TX)
- ESP32 GND → STM32 GND

UART parameters:

- Baud rate: 115200 bps
- Data bits: 8
- Parity: None
- Stop bits: 1

### Relay Module Connection

Connect relay module to GPIO18 (or configured pin):

- ESP32 GPIO18 → Relay module control input
- ESP32 GND → Relay module GND
- External power supply → Relay module VCC (typically 3.3V)

Note: Use appropriate level shifting if relay module requires 5V logic levels.

## MQTT Topic Structure

The firmware uses a structured topic hierarchy for organized communication:

### Subscribed Topics (ESP32 receives commands)

| Topic                            | Purpose                       | Payload Example          |
| -------------------------------- | ----------------------------- | ------------------------ |
| `datalogger/stm32/command`       | STM32 sensor control commands | `SINGLE`                 |
| `datalogger/esp32/relay/control` | Relay control commands        | `RELAY ON` / `RELAY OFF` |

### Published Topics (ESP32 sends data)

| Topic                            | Purpose                 | Payload Example                                                                   |
| -------------------------------- | ----------------------- | --------------------------------------------------------------------------------- |
| `datalogger/stm32/single/data`   | Single sensor reading   | `{"mode":"SINGLE","timestamp":1728930680,"temperature":23.45,"humidity":65.20}`   |
| `datalogger/stm32/periodic/data` | Periodic sensor reading | `{"mode":"PERIODIC","timestamp":1728930680,"temperature":23.45,"humidity":65.20}` |
| `datalogger/esp32/system/state`  | ESP32 system status     | `{"device":"ON","periodic":"ON"}`                                                 |
| `datalogger/esp32/relay/control` | Relay state feedback    | `RELAY ON` / `RELAY OFF`                                                          |

### Command Examples

**STM32 Sensor Control Commands:**

```bash
# Single measurement
mosquitto_pub -h 192.168.1.100 -t "datalogger/stm32/command" -m "SINGLE"

# Start periodic measurements
mosquitto_pub -h 192.168.1.100 -t "datalogger/stm32/command" -m "PERIODIC ON"

# Stop periodic measurements
mosquitto_pub -h 192.168.1.100 -t "datalogger/stm32/command" -m "PERIODIC OFF"
```

**Relay Control Commands:**

```bash
# Turn relay on
mosquitto_pub -h 192.168.1.100 -t "datalogger/esp32/relay/control" -m "RELAY ON"

# Turn relay off
mosquitto_pub -h 192.168.1.100 -t "datalogger/esp32/relay/control" -m "RELAY OFF"
```

**Subscribe to Data:**

```bash
# Subscribe to single sensor readings
mosquitto_sub -h 192.168.1.100 -t "datalogger/stm32/single/data"

# Subscribe to periodic sensor readings
mosquitto_sub -h 192.168.1.100 -t "datalogger/stm32/periodic/data"

# Subscribe to system state
mosquitto_sub -h 192.168.1.100 -t "datalogger/esp32/system/state"

# Subscribe to relay state feedback
mosquitto_sub -h 192.168.1.100 -t "datalogger/esp32/relay/control"
```

## System Operation

### Startup Sequence

1. ESP32 initializes WiFi connection
2. WiFi manager establishes connection to configured access point
3. MQTT client connects to broker using configured credentials
4. Subscribe to command topics
5. Initialize UART communication with STM32
6. Start FreeRTOS tasks for UART reception and command processing
7. Publish initial status message
8. Enter main operation loop

### Data Flow: STM32 to MQTT

1. STM32 transmits JSON-formatted sensor data via UART
2. ESP32 UART interrupt stores received bytes in ring buffer
3. UART receive task reads complete lines from ring buffer
4. JSON parser validates and extracts temperature and humidity values
5. MQTT handler publishes data to appropriate topics
6. Web dashboard receives and displays data

### Command Flow: MQTT to STM32

1. MQTT broker receives command from web dashboard or external client
2. ESP32 MQTT handler receives message on subscribed topic
3. Command parser validates and formats command string
4. UART transmit function sends command to STM32
5. STM32 executes command and responds with data
6. Response follows STM32-to-MQTT data flow

### State Management

The system maintains state for:

- Relay on/off status
- Current measurement mode (idle, single, periodic)
- WiFi connection status
- MQTT connection status
- Last sensor readings

State is synchronized through:

- Retained MQTT messages for persistence
- State request/response mechanism
- Automatic state updates on changes

## Performance Characteristics

| Metric                 | Typical Value              |
| ---------------------- | -------------------------- |
| Memory Usage (RAM)     | ~8 KB for core firmware    |
| Memory Usage (Flash)   | ~15 KB for core firmware   |
| CPU Utilization        | < 5% at 1 Hz sampling rate |
| UART to MQTT Latency   | < 50 ms typical            |
| Maximum Sampling Rate  | 10 Hz continuous           |
| MQTT Reconnection Time | 2-5 seconds typical        |
| WiFi Reconnection Time | 5-10 seconds typical       |

Note: Actual values depend on network conditions, configured buffer sizes, and enabled features.

## Troubleshooting

### Common Issues

**UART Communication Failure**

Symptoms: No data received from STM32, commands not executed

Solutions:

- Verify TX and RX pin connections (cross-connected: ESP32 TX to STM32 RX)
- Check common ground connection between ESP32 and STM32
- Confirm baud rate matches on both devices (115200 bps)
- Monitor for electrical noise or loose connections
- Use idf.py monitor to check for UART error messages

**MQTT Connection Problems**

Symptoms: Cannot connect to broker, frequent disconnections

Solutions:

- Verify MQTT broker is running and accessible on network
- Check broker URL, username, and password in menuconfig
- Confirm broker supports MQTT v5 protocol
- Verify network firewall allows MQTT port (default 1883)
- Check for duplicate client IDs if multiple ESP32 devices
- Monitor broker logs for authentication or authorization errors

**WiFi Connection Issues**

Symptoms: Cannot connect to access point, frequent WiFi drops

Solutions:

- Verify SSID and password are correct in menuconfig
- Check WiFi signal strength and stability
- Confirm ESP32 is within range of access point
- Try fixed IP address if DHCP is problematic
- Check for WiFi channel congestion
- Monitor ESP32 serial output for WiFi error codes

**State Synchronization Problems**

Symptoms: Web dashboard shows incorrect state, relay state mismatch

Solutions:

- Verify retained messages are enabled on MQTT broker
- Send state request command to force state update
- Check MQTT topic subscriptions are correct
- Verify JSON formatting in state messages
- Clear retained messages on broker and restart system

**Memory Issues**

Symptoms: System crashes, heap allocation failures

Solutions:

- Monitor heap usage: esp_get_free_heap_size()
- Reduce ring buffer size in menuconfig if needed
- Check for memory leaks in custom code
- Verify stack sizes for FreeRTOS tasks are adequate
- Enable heap poisoning debug feature

## Testing and Debugging

### System Testing

Monitor all MQTT topics:

```bash
mosquitto_sub -h 192.168.1.100 -t "datalogger/#" -v
```

Test sensor commands:

```bash
# Send command and observe output
mosquitto_pub -h 192.168.1.100 -t "datalogger/esp32/command" -m "SINGLE"

# Subscribe to data topics
mosquitto_sub -h 192.168.1.100 -t "datalogger/stm32/data/#"
```

Test relay control:

```bash
mosquitto_pub -h 192.168.1.100 -t "datalogger/esp32/relay" -m "RELAY ON"
mosquitto_pub -h 192.168.1.100 -t "datalogger/esp32/relay" -m "RELAY OFF"
```

### Debug Logging

Enable detailed logging via menuconfig:

```
Component config → Log output → Default log verbosity → Debug
```

Monitor ESP32 logs with filtering:

```bash
# All logs
idf.py monitor

# Filter by component
idf.py monitor | grep "MQTT_HANDLER"
idf.py monitor | grep "SENSOR_PARSER"
idf.py monitor | grep "STM32_UART"
```

### Component Testing

Test individual components:

```bash
cd components/ring_buffer
idf.py build

cd ../json_sensor_parser
idf.py build
```

### Memory Monitoring

Check heap usage at runtime (add to code):

```c
#include "esp_heap_caps.h"

uint32_t free_heap = esp_get_free_heap_size();
ESP_LOGI(TAG, "Free heap: %d bytes", free_heap);
```

## Component Documentation

Each component in the components/ directory contains its own README.md file with detailed information:

- Button Handler Details: [Button input configuration and debouncing](components/button_handler/README.md)
- CoAP Handler Details: [CoAP protocol implementation](components/coap_handler/README.md)
- JSON Sensor Parser Details: [JSON parsing logic and data validation](components/json_sensor_parser/README.md)
- JSON Utilities Details: [JSON utility function reference](components/json_utils/README.md)
- MQTT Handler Details: [MQTT client API and configuration](components/mqtt_handler/README.md)
- Relay Control Details: [Relay control commands and GPIO setup](components/relay_control/README.md)
- Ring Buffer Details: [Circular buffer implementation and usage](components/ring_buffer/README.md)
- STM32 UART Details: [UART communication protocol and configuration](components/stm32_uart/README.md)
- WiFi Manager Details: [WiFi connection management and status](components/wifi_manager/README.md)

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
