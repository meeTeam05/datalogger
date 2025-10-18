# Firmware

## Overview

This directory contains firmware for both STM32 and ESP32 microcontrollers in the datalogger system.

## Architecture

```
SHT3X Sensor ←→ STM32 CLI Interface ←[UART]→ ESP32 MQTT Bridge ←[WiFi]→ Web Dashboard
                       ↓                            ↓
                 Local Control via           GPIO Relay Control
                  Serial Terminal
```

## Components

### STM32 Firmware 
**Primary sensor control and data acquisition**

- Hardware: STM32F1xx + SHT3X temperature/humidity sensor
- Interface: Command-line interface via UART (115200 baud)
- Features: Single-shot measurements, continuous monitoring (0.5-10Hz), heater control
- Architecture: Ring buffer + command parser + I2C sensor driver
- Data Output: Real-time temperature/humidity readings with timestamp

Key Capabilities:
- Precision control: HIGH/MEDIUM/LOW repeatability settings
- Multiple sampling modes: Single-shot and periodic (0.5, 1, 2, 4, 10 Hz)
- Built-in heater management for condensation prevention
- Robust I2C communication with CRC validation
- Local serial terminal control for debugging

### ESP32 Firmware
**IoT connectivity and remote control**

- Hardware: ESP32 DevKit + relay module
- Connectivity: WiFi + MQTT5 protocol  
- Features: Bidirectional STM32-Web communication, relay control, real-time data streaming
- Architecture: Modular components for UART, MQTT, parsing, and GPIO control
- Integration: Transparent bridge between STM32 CLI and web applications

Key Capabilities:
- Command forwarding: Web to MQTT to ESP32 to STM32
- Data streaming: STM32 to ESP32 to MQTT to Web dashboard
- Device control: Remote relay switching via GPIO
- State synchronization: Real-time system status monitoring
- Auto-reconnection and error recovery

## Configuration

1. **STM32 Setup**
```bash
cd STM32/
# Configure via STM32CubeMX, build with your preferred toolchain
# Connect SHT3X: SCL→PB6, SDA→PB7, VCC→3.3V, GND→GND
# Connect UART: TX→PA9, RX→PA10
```

2. **ESP32 Setup**  
```bash
cd ESP32/
idf.py menuconfig  # Configure WiFi, MQTT broker, GPIO pins
idf.py build flash monitor
# Connect to STM32: GPIO17→STM32_RX, GPIO16←STM32_TX, GND→GND
```

3. **System Integration**
```bash
# Test STM32 locally via serial terminal
echo "SHT3X SINGLE HIGH" > /dev/ttyUSB0

# Test ESP32 bridge via MQTT
mosquitto_pub -t "esp32/sensor/sht3x/command" -m "SHT3X PERIODIC 1 HIGH"
mosquitto_sub -t "esp32/sensor/sht3x/periodic/+"
```

## Communication Protocol

### Command Interface
| Source | Command | Target | Result |
|--------|---------|--------|--------|
| Serial/Web | `SHT3X SINGLE HIGH` | STM32 | Single measurement |
| Serial/Web | `SHT3X PERIODIC 1 HIGH` | STM32 | 1Hz continuous sampling |
| Serial/Web | `SHT3X HEATER ENABLE` | STM32 | Enable sensor heater |
| Web | `RELAY ON` | ESP32 | GPIO relay control |

### Data Flow
#### Periodic Flow
```
STM32 Output: "PERIODIC 23.45 65.20"
      ↓
ESP32 Parsing: temperature=23.45, humidity=65.20  
      ↓
MQTT Topics:
  - esp32/sensor/sht3x/periodic/temperature → "23.45"
  - esp32/sensor/sht3x/periodic/humidity → "65.20"
```
#### Single Flow
```
STM32 Output: "Single 23.45 65.20"
      ↓
ESP32 Parsing: temperature=23.45, humidity=65.20  
      ↓
MQTT Topics:
  - esp32/sensor/sht3x/single/temperature → "23.45"
  - esp32/sensor/sht3x/single/humidity → "65.20"
```

### MQTT Topics
| Topic | Direction | Purpose | Example |
|-------|-----------|---------|---------|
| `esp32/sensor/sht3x/command` | Subscribe | Sensor control | `SHT3X SINGLE HIGH` |
| `esp32/sensor/sht3x/single/temperature` | Publish | Single-shot temp | `23.45` |
| `esp32/sensor/sht3x/periodic/humidity` | Publish | Continuous humidity | `65.20` |
| `esp32/control/relay` | Subscribe | Relay control | `ON` / `OFF` |
| `esp32/status` | Publish | System status | `{"device":"ON","wifi":"connected"}` |

## Technical Specifications

### Performance Metrics
| Metric | STM32 | ESP32 | System |
|--------|-------|-------|---------|
| **Memory Usage** | <1KB RAM | ~8KB RAM | - |
| **Response Time** | <100ms | <50ms | <150ms end-to-end |
| **Max Sample Rate** | 10Hz | 10Hz | 10Hz continuous |
| **Power Consumption** | ~50mA | ~200mA | <300mA total |

### Communication Specs
- **UART**: 115200 baud, 8N1, hardware flow control disabled
- **I2C**: 100kHz standard mode, 7-bit addressing
- **WiFi**: 802.11 b/g/n, WPA2/WPA3 security
- **MQTT**: v5.0 protocol, QoS 1, retained messages for state

## Deployment Scenarios

### **Laboratory Monitoring**
- STM32 provides precise, calibrated measurements
- ESP32 enables remote monitoring without PC connection
- Web dashboard for multiple sensor management

### **Industrial IoT**
- Relay control for automated ventilation/heating
- Real-time alerts via MQTT integration
- Scalable to multiple sensor nodes

### **Development & Testing**
- Direct STM32 control via serial terminal
- MQTT bridge for web application development
- Isolated testing of sensor algorithms

## Advanced Configuration

### Security Hardening
```bash
# Enable MQTT TLS (ESP32)
idf.py menuconfig → ESP32 MQTT5 Bridge → Enable TLS

# Implement certificate authentication
# Add custom CA certificates for broker validation
```

### Performance Optimization
```c
// STM32: Adjust periodic fetch interval
#define timeData 1000  // 1 second instead of 5

// ESP32: Optimize MQTT parameters
CONFIG_MQTT_BUFFER_SIZE=2048
CONFIG_MQTT_TASK_STACK_SIZE=8192
```

### Multiple Sensor Support
- Extend command parser for sensor addressing
- Add device discovery via MQTT topics
- Implement sensor health monitoring

## Troubleshooting

### Common Issues
| Problem | Component | Solution |
|---------|-----------|----------|
| No sensor data | STM32 | Check I2C wiring, verify sensor address |
| UART communication failure | Both | Verify cross-connect wiring, baud rate |
| MQTT connection drops | ESP32 | Check WiFi stability, broker availability |
| Web dashboard not updating | System | Verify MQTT topic subscriptions |

### Debug Tools
```bash
# STM32 debugging
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg

# ESP32 monitoring  
idf.py monitor | grep -E "(MQTT|UART|SENSOR)"

# MQTT broker testing
mosquitto_sub -v -t "esp32/+/+/+"
```

## Support

- **Hardware Issues**: Check component READMEs for detailed troubleshooting
- **Software Integration**: Review communication protocol specifications
- **Custom Development**: Each component supports independent modification and extension

---

**Quick Reference:**
- STM32 Commands: 22 total sensor control commands
- ESP32 Topics: 8+ MQTT topics for full system control  
- Update Rate: Up to 10Hz continuous monitoring
- Latency: <150ms web-to-sensor command execution

## License

MIT License - see project root for details.