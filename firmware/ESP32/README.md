# ESP32 MQTT Bridge Firmware

ESP32 firmware for bridging STM32-based SHT31 temperature/humidity sensors to MQTT networks with real-time web dashboard control.

## Architecture Overview

```
MQTT Broker ←→ ESP32 MQTT Handler ←→ STM32 UART Interface ←→ STM32
                      ↓
             State Management & Data Processing
                      ↓
                 Relay Control (GPIO)
```

## Project Structure

```
esp32_mqtt_bridge/
├── main/
│   ├── app_main.c                        # Main application logic
│   ├── CMakeLists.txt                    # Main component build
│   └── Kconfig.projbuild                 # Configuration options
├── components/
│   ├── ring_buffer/                      # Circular buffer for UART
│   ├── stm32_uart/                       # STM32 communication layer  
│   ├── mqtt_handler/                     # MQTT5 client wrapper
│   ├── relay_control/                    # GPIO relay management
│   ├── sensor_parser/                    # SHT3X data parsing
│   ├── wifi_manager/                     # Wifi configure
│   └── protocol_examples_common/         # Protocol Common
├── CMakeLists.txt                        # Root build configuration
└── README.md
```

## Component Architecture

### Core Components

**Ring Buffer** (`components/ring_buffer/`)
- Thread-safe circular buffer for UART data
- 256-byte default capacity (configurable)
- Volatile pointers for interrupt safety

**STM32 UART** (`components/stm32_uart/`)
- Asynchronous UART communication
- Line-based data parsing with noise filtering
- Dedicated receive task with ring buffer
- Command transmission with proper termination

**MQTT Handler** (`components/mqtt_handler/`)
- MQTT5 protocol implementation
- Auto-generated client ID from MAC address
- Connection state management with auto-reconnect
- Event-driven callback system

**Relay Control** (`components/relay_control/`)
- GPIO-based relay switching
- Multiple command formats support
- State change notifications
- Safe initialization/cleanup

**Sensor Parser** (`components/sensor_parser/`)
- SHT3X data format validation
- Separate handling for SINGLE/PERIODIC modes
- Temperature range: -40°C to 125°C
- Humidity range: 0% to 100%

**Wifi Manager** (`components/wifi_manager/`)
- Configure wifi for ESP

## Quick Start

### Prerequisites
- ESP-IDF v5.0+
- MQTT broker with WebSocket support
- STM32 device with SHT31 sensor

### Build Steps

1. **Clone and Setup**
```bash
mkdir esp32_mqtt_bridge && cd esp32_mqtt_bridge
# Copy project files according to structure above
```

2. **Configure Project**
```bash
idf.py menuconfig
```

Key configuration sections:
- **ESP32 MQTT5 Bridge Configuration**: Broker settings, UART pins
- **Example Connection Configuration**: WiFi credentials

3. **Build and Flash**
```bash
idf.py build flash monitor
```

## Configuration

### MQTT Settings
```c
// Default values (configurable via menuconfig)
CONFIG_BROKER_URL="mqtt://192.168.1.100"
CONFIG_MQTT_USERNAME="DataLogger"  
CONFIG_MQTT_PASSWORD="datalogger"
```

### Hardware Settings  
```c
CONFIG_MQTT_UART_PORT_NUM=2       // UART2
CONFIG_MQTT_UART_TXD=17           // TX pin
CONFIG_MQTT_UART_RXD=16           // RX pin  
CONFIG_MQTT_UART_BAUD_RATE=115200 // Baud rate
CONFIG_RELAY_GPIO_NUM=4           // Relay control pin
```

## MQTT Protocol

### Topics Structure

| Direction | Topic | Purpose | Example |
|-----------|--------|---------|---------|
| Subscribe | `esp32/sensor/sht3x/command` | Sensor commands | `SHT3X SINGLE HIGH` |
| Subscribe | `esp32/control/relay` | Relay control | `RELAY ON` |
| Subscribe | `esp32/state` | State synchronization | `REQUEST` |
| Publish | `esp32/sensor/sht3x/single/temperature` | Single temp reading | `23.45` |
| Publish | `esp32/sensor/sht3x/periodic/humidity` | Periodic humidity | `67.8` |
| Publish | `esp32/state` | System state | `{"device":"ON","periodic":"OFF"}` |

### Command Examples

**Sensor Control**
```bash
# Single measurement
mosquitto_pub -t "esp32/sensor/sht3x/command" -m "SHT3X SINGLE HIGH"

# Periodic measurement (1Hz)  
mosquitto_pub -t "esp32/sensor/sht3x/command" -m "SHT3X PERIODIC 1 HIGH"

# Stop periodic mode
mosquitto_pub -t "esp32/sensor/sht3x/command" -m "SHT3X PERIODIC STOP"
```

**Device Control**
```bash
# Turn device on/off
mosquitto_pub -t "esp32/control/relay" -m "RELAY ON"
mosquitto_pub -t "esp32/control/relay" -m "RELAY OFF"
```

**State Synchronization**
```bash
# Request current state
mosquitto_pub -t "esp32/state" -m "REQUEST"

# Monitor state changes
mosquitto_sub -t "esp32/state"
```

## Data Flow

### Command Processing
```
Web Dashboard → MQTT Broker → ESP32 MQTT Handler
       ↓
ESP32 Main App → STM32 UART → STM32 Device
```

### Sensor Data Publishing
```
STM32 Device → UART → Ring Buffer → Sensor Parser
       ↓
Main App Callbacks → MQTT Handler → MQTT Broker → Web Dashboard
```

### State Synchronization
```
Hardware State Change → Update Global State → Publish State Message
       ↓
Web Dashboard Receives → Updates UI → Stays in Sync
```

## Performance Specifications

| Metric | Value |
|--------|--------|
| Memory Usage | ~8KB RAM, ~15KB Flash |
| CPU Utilization | <5% at 1Hz sampling |
| UART-to-MQTT Latency | <50ms |
| Max Sampling Rate | 10Hz continuous |
| MQTT Reconnect Time | ~2-5 seconds |

## Error Handling

### UART Communication
- Noise filtering and line validation
- Buffer overflow protection
- Automatic retry on transmission failure

### MQTT Connection
- Automatic reconnection with exponential backoff  
- Connection state monitoring
- Message queuing during disconnection

### State Management
- Retained message support for state persistence
- Duplicate message prevention
- Hardware/software state synchronization

## Testing

### Component Testing
```bash
# Test individual components
cd components/ring_buffer && idf.py build
cd components/sensor_parser && idf.py build
```

### System Testing
```bash
# Monitor all MQTT topics
mosquitto_sub -h broker-ip -t "esp32/+/+/+"

# Simulate STM32 data (via UART)
echo "SINGLE 25.7 62.3" > /dev/ttyUSB0
echo "PERIODIC 26.1 64.8" > /dev/ttyUSB0
```

### Debug Output
```bash
# Monitor ESP32 logs
idf.py monitor

# Filter specific components  
idf.py monitor | grep "MQTT_HANDLER"
idf.py monitor | grep "SENSOR_PARSER"
```

## Troubleshooting

### Common Issues

**UART Communication Failure**
- Check wiring: TX→RX, RX→TX, GND→GND
- Verify baud rate matches STM32 configuration
- Monitor for electrical noise/interference

**MQTT Connection Issues**  
- Confirm broker supports MQTT5 and WebSockets
- Check network connectivity and firewall rules
- Verify credentials and broker URL

**State Synchronization Problems**
- Ensure retained messages are enabled on broker
- Check for duplicate client IDs causing disconnections
- Monitor state topic for proper JSON formatting

**Memory Issues**
- Reduce ring buffer size if needed
- Monitor heap usage: `esp_get_free_heap_size()`
- Check for memory leaks in custom code

### Debug Configuration
```c
// Enable debug logging (menuconfig)
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
CONFIG_LOG_MAXIMUM_LEVEL_DEBUG=y

// Component-specific logging
CONFIG_LOG_DEFAULT_LEVEL_MQTT_HANDLER=4  // DEBUG
CONFIG_LOG_DEFAULT_LEVEL_SENSOR_PARSER=4 // DEBUG
```

## Extending the Firmware

### Adding New Sensors
1. Create new parser component following sensor_parser structure
2. Add topic definitions in app_main.c
3. Register callbacks for data processing

### Custom Commands
1. Extend MQTT topic handling in `on_mqtt_data_received()`
2. Add command parsing logic
3. Implement hardware control functions

### Additional Hardware
1. Create new control component (follow relay_control pattern)
2. Add GPIO initialization in main app
3. Register MQTT command handlers

## Production Considerations

### Security
- Use TLS/SSL for MQTT connections (port 8883)
- Implement device authentication
- Regular security updates for ESP-IDF

### Reliability  
- Implement watchdog timers
- Add OTA update capability
- Monitor system health metrics

### Scalability
- Support multiple sensor instances
- Implement device discovery protocols
- Add configuration management

**Hardware Requirements:**
- ESP32 DevKit or compatible board
- STM32 with SHT31 sensor
- 5V relay module (optional)
- 3.3V/5V level shifters (if needed)

**Software Dependencies:**
- ESP-IDF v5.0+
- MQTT broker (Mosquitto recommended)
- Web dashboard (see companion web project)

## License

MIT License - see project root for details.
