# DATALOGGER - IoT Environmental Monitoring System

A complete production-grade IoT data acquisition system for environmental monitoring with real-time visualization, cloud storage, and remote device control capabilities.

## System Architecture

```
┌─────────────┐    I2C     ┌─────────────┐    UART      ┌─────────────┐    WiFi/MQTT    ┌──────────────┐
│   SHT3X     │────────────│    STM32    │──────────────│    ESP32    │─────────────────│ MQTT Broker  │
│   Sensor    │            │  F103C8T6   │   115200bps  │  Gateway    │  ws://x.x.x.x   │ (Mosquitto)  │
└─────────────┘            └──────┬──────┘              └──────┬──────┘                 └──────┬───────┘
                                  │                            │                               │
                           ┌──────┴──────┐              ┌──────┴──────┐                        │
                           │  DS3231 RTC │              │ Relay GPIO4 │                        │
                           │  Timestamp  │              │   Control   │                        │
                           └──────┬──────┘              └─────────────┘                        │
                                  │                                                            │
                           ┌──────┴──────┐                                             ┌───────┴────────┐
                           │  SD Card    │                                             │  Web Dashboard │
                           │  Buffer     │                                             │  + Firebase DB │
                           │  (Offline)  │                                             └────────────────┘
                           └──────┬──────┘
                                  │
                           ┌──────┴──────┐
                           │  ILI9225    │
                           │  Display    │
                           │  176x220px  │
                           └─────────────┘
```

**Communication Flow:**

- **Upstream (Data)**: STM32 → UART (115200bps) → ESP32 → MQTT (QoS 0/1) → Web Dashboard
- **Downstream (Commands)**: Web Dashboard → MQTT → ESP32 → UART → STM32 → Sensor/Relay
- **Offline Buffering**: STM32 → SD Card (204,800 records) → Sync when ESP32 reconnects
- **Cloud Storage**: MQTT → Firebase Realtime Database → Historical data retrieval

## Overview

This project implements a **production-ready environmental monitoring system** designed for industrial, laboratory, and IoT applications.

### Core Capabilities

- **Real-time sensor monitoring** with SHT3X temperature/humidity sensors (±0.2°C, ±2% RH accuracy)
- **Offline data buffering** to SD card (204,800 records capacity) during network outages
- **Automatic data synchronization** when connectivity is restored
- **Real-time visualization** via web dashboard with Chart.js line charts
- **Cloud data persistence** with Firebase Realtime Database integration
- **Remote device control** via MQTT commands (relay switching, measurement modes)
- **Local LCD display** (ILI9225 176x220) showing live readings and system status
- **Accurate timestamping** using DS3231 RTC module with battery backup

### System Components

1. **STM32F103C8T6 Firmware** - Data acquisition, sensor interfacing, SD buffering, and display control
2. **ESP32 Gateway** - WiFi/MQTT bridge, relay control, and UART communication
3. **Mosquitto MQTT Broker** - Message routing with WebSocket support (ports 1883, 8083)
4. **Web Dashboard** - Real-time monitoring, historical data analysis, and device control interface

## Key Features

### Data Acquisition & Storage

- **High-accuracy sensing**: SHT3X sensor with ±0.2°C temperature and ±2% RH humidity accuracy
- **Flexible sampling rates**: Configurable from 1 second to 60 minutes interval
- **Offline data buffering**: SD card storage with 204,800 records capacity during network outages
- **Automatic synchronization**: Buffered data automatically syncs when connectivity is restored
- **Accurate timestamping**: DS3231 RTC with battery backup (±2ppm accuracy, 0-40°C)
- **Local display**: ILI9225 LCD (176x220px) showing live temperature, humidity, timestamp, and system status

### Communication & Control

- **MQTT v5 protocol**: Robust messaging with QoS 0/1 support for reliable data delivery
- **WebSocket support**: Real-time web client connectivity on port 8083
- **Remote relay control**: GPIO-based switching for connected devices (pumps, fans, alarms)
- **Command interface**: Text-based command protocol over UART
- **Interrupt-driven UART**: Efficient 115200bps STM32↔ESP32 communication with 256-byte ring buffer
- **FreeRTOS task management**: Priority-based processing on ESP32 gateway

### Web Dashboard & Visualization

- **Real-time monitoring**: Live sensor data updates via MQTT WebSocket
- **Real-time charts**: Chart.js line graphs with live sensor data updates (50-point rolling window)
- **Firebase integration**: Cloud database for historical data storage and retrieval
- **Component health monitoring**: Live status indicators for all system components
- **Multi-mode operation**: Single shot and periodic monitoring modes
- **Time synchronization**: Internet NTP or manual time setting with RTC update
- **Responsive design**: Works on desktop and mobile browsers

### Reliability & Security

- **Bcrypt authentication**: Secure MQTT broker access with encrypted passwords
- **Persistent messaging**: MQTT message storage survives broker restarts
- **Automatic reconnection**: Intelligent retry logic for WiFi and MQTT failures
- **Error handling**: Comprehensive sensor failure detection and recovery
- **Modular architecture**: 18 firmware library modules, 9 ESP32 components
- **Production-ready**: Extensively tested with detailed documentation

## Hardware Requirements

### Electronic Components

#### Core Microcontrollers

- **STM32F103C8T6** "Blue Pill" or equivalent (ARM Cortex-M3, 72MHz, 64KB Flash, 20KB RAM)
- **ESP32-WROOM-32** or ESP32-DevKitC (Dual-core Xtensa LX6, 240MHz, WiFi 802.11 b/g/n)

#### Sensors & Peripherals

- **SHT30/SHT31/SHT35** temperature/humidity sensor (I2C, ±0.2°C, ±2% RH accuracy)
- **DS3231** high-precision RTC module (I2C, ±2ppm accuracy, battery backup)
- **MicroSD card module** (SPI, FAT32 formatted, 1GB+ recommended for buffering)
- **ILI9225 LCD display** (176x220px, 2.2", SPI interface) - for local status display
- **Relay module** (optional, for device control - pumps, fans, alarms)

#### Programming & Debug Tools

- **ST-Link V2** programmer for STM32 (or compatible JTAG/SWD debugger)
- **USB-to-TTL adapter** for UART debugging (CP2102, CH340, or FTDI)

#### Power Supply

- **5V DC power supply** (2A minimum recommended for entire system)
- **CR2032 battery** for DS3231 RTC backup

### Pin Connections

#### STM32 Pinout Configuration

**I2C Bus (Shared: SHT3X + DS3231)**

| STM32 Pin | I2C Function | Connected Devices           | Notes                  |
| --------- | ------------ | --------------------------- | ---------------------- |
| PB6       | I2C1_SCL     | SHT3X (0x44), DS3231 (0x68) | 4.7kΩ pull-up required |
| PB7       | I2C1_SDA     | SHT3X, DS3231               | 4.7kΩ pull-up required |

**UART Communication (STM32 ↔ ESP32)**

| STM32 Pin | UART Function | ESP32 Pin    | Signal Direction |
| --------- | ------------- | ------------ | ---------------- |
| PA9       | USART1_TX     | GPIO16 (RX2) | STM32 → ESP32    |
| PA10      | USART1_RX     | GPIO17 (TX2) | ESP32 → STM32    |
| GND       | Ground        | GND          | Common ground    |

**SPI1 – SD Card Interface**

| STM32 Pin | SPI Function | SD Card Pin |
| --------- | ------------ | ----------- |
| PA5       | SPI1_SCK     | CLK         |
| PA6       | SPI1_MISO    | MISO        |
| PA7       | SPI1_MOSI    | MOSI        |
| PA4       | GPIO (SD CS) | CS          |

**SPI2 – ILI9225 TFT Display Interface**

| STM32 Pin | SPI Function   | ILI9225 Pin |
| --------- | -------------- | ----------- |
| PB13      | SPI2_SCK       | SCK         |
| PB15      | SPI2_MOSI      | SDA         |
| PA11      | GPIO (LCD RS)  | RS          |
| PA8       | GPIO (LCD RST) | RST         |
| PB12      | GPIO (LCD CS)  | CS          |

**Power Distribution**

| STM32 Pin | Function     | Notes                             |
| --------- | ------------ | --------------------------------- |
| 3.3V      | Power output | For SHT3X, DS3231, SD card logic  |
| 5V        | Power input  | From USB or external 5V regulator |
| GND       | Ground       | Common ground for all modules     |

#### ESP32 Pinout Configuration

**UART Communication (ESP32 ↔ STM32)**

| ESP32 Pin | UART Function | STM32 Pin | Signal Direction |
| --------- | ------------- | --------- | ---------------- |
| GPIO16    | UART2_RX      | PA9 (TX)  | STM32 → ESP32    |
| GPIO17    | UART2_TX      | PA10 (RX) | ESP32 → STM32    |
| GND       | Ground        | GND       | Common ground    |

**Relay Control**

| ESP32 Pin             | Function       | Relay Module Pin | Signal Type        |
| --------------------- | -------------- | ---------------- | ------------------ |
| GPIO18 (configurable) | Digital Output | IN               | Active HIGH (3.3V) |
| 5V                    | Power supply   | VCC              | Relay power        |
| GND                   | Ground         | GND              | Common ground      |

**Power Supply**

| ESP32 Pin | Function     | Notes                             |
| --------- | ------------ | --------------------------------- |
| 5V (VIN)  | Power input  | From USB or external 5V regulator |
| 3.3V      | Power output | Internal LDO, max 500mA           |
| GND       | Ground       | Common ground                     |

## Quick Start

### 1. MQTT Broker Setup

Create the required directory structure and user credentials:

```bash
mkdir -p broker/config/auth broker/data broker/log
docker run --rm -v "$PWD/broker/config/auth:/work" eclipse-mosquitto:2 \
  mosquitto_passwd -c /work/passwd.txt DataLogger
```

Start the MQTT broker with Docker:

```bash
docker run -d --name mqtt-broker \
  -p 1883:1883 \
  -p 8083:8083 \
  -v "$PWD/broker/mosquitto.conf:/mosquitto/config/mosquitto.conf" \
  -v "$PWD/broker/config/auth:/mosquitto/config/auth" \
  -v "$PWD/broker/data:/mosquitto/data" \
  -v "$PWD/broker/log:/mosquitto/log" \
  eclipse-mosquitto:2
```

The broker will listen on:

- Port 1883: Standard MQTT protocol (for ESP32)
- Port 8083: WebSocket protocol (for web dashboard)

### 2. STM32 Firmware Installation

Using STM32CubeIDE:

1. Open the project directory: `firmware/STM32/`
2. Build the project: Project → Build All
3. Connect ST-Link programmer to STM32
4. Flash firmware: Run → Debug

Using command-line GCC toolchain:

```bash
cd firmware/STM32/
make clean && make all
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/STM32_DATALOGGER.elf verify reset exit"
```

### 3. ESP32 Firmware Installation

Configure WiFi and MQTT settings:

```bash
cd firmware/ESP32/
idf.py menuconfig
```

Navigate to configuration sections:

- WiFi Connection Configuration: Set WiFi SSID and password
- IoT Protocol Configuration → MQTT Protocol Configuration: Set MQTT broker URI, username, password

Build and flash:

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor  # Replace with your port (COM3 on Windows)
```

### 4. Web Dashboard Deployment

For development:

```bash
cd web/
python -m http.server 8080
```

For production (using Nginx):

```bash
# Copy web files to Nginx root
sudo cp -r web/* /var/www/html/datalogger/

# Configure Nginx (see web/README.md for details)
sudo systemctl restart nginx
```

Access at http://localhost:8080 and configure MQTT broker settings in the dashboard.

## MQTT Communication Protocol

### Topic Structure

| Topic                          | Direction       | QoS | Payload Type | Purpose                             |
| ------------------------------ | --------------- | --- | ------------ | ----------------------------------- |
| `datalogger/esp32/command`     | Web/App → STM32 | 1   | Text         | Sensor control commands             |
| `datalogger/esp32/relay`       | Web/App → ESP32 | 1   | Text         | Relay on/off control                |
| `datalogger/esp32/status`      | ESP32 → Web/App | 0   | JSON         | System status                       |
| `datalogger/esp32/sensor/data` | STM32 → Web/App | 0   | JSON         | Sensor readings (single & periodic) |
| `datalogger/esp32/relay/state` | ESP32 → Web/App | 0   | Text         | Relay state feedback                |

### Command Examples

**Single Sensor Read:**

```bash
mosquitto_pub -h [broker-ip] -p 1883 -t 'datalogger/esp32/command' \
  -u DataLogger -P [password] -m 'SINGLE'
```

**Start Periodic Mode (5 second interval):**

```bash
mosquitto_pub -h [broker-ip] -p 1883 -t 'datalogger/esp32/command' \
  -u DataLogger -P [password] -m 'PERIODIC ON'
```

**Set Periodic Interval (10 seconds):**

```bash
mosquitto_pub -h [broker-ip] -p 1883 -t 'datalogger/esp32/command' \
  -u DataLogger -P [password] -m 'SET PERIODIC INTERVAL 10000'
```

**Stop Periodic Mode:**

```bash
mosquitto_pub -h [broker-ip] -p 1883 -t 'datalogger/esp32/command' \
  -u DataLogger -P [password] -m 'PERIODIC OFF'
```

**Relay Control:**

```bash
# Turn ON
mosquitto_pub -h [broker-ip] -p 1883 -t 'datalogger/esp32/relay' \
  -u DataLogger -P [password] -m 'ON'

# Turn OFF
mosquitto_pub -h [broker-ip] -p 1883 -t 'datalogger/esp32/relay' \
  -u DataLogger -P [password] -m 'OFF'
```

**Set RTC Time:**

```bash
# Option 1: Unix timestamp
mosquitto_pub -h [broker-ip] -p 1883 -t 'datalogger/esp32/command' \
  -u DataLogger -P [password] -m 'SET TIME 1760000000'

# Option 2: Date/time format (YYYY MM DD HH MM SS)
mosquitto_pub -h [broker-ip] -p 1883 -t 'datalogger/esp32/command' \
  -u DataLogger -P [password] -m 'SET TIME 2025 10 28 14 30 00'
```

### Data Output Format

**Sensor Data Message (STM32 → ESP32 → MQTT):**

```json
{
  "mode": "PERIODIC",
  "timestamp": 1729699200,
  "temperature": 25.5,
  "humidity": 60.0
}
```

**System Status Message:**

```json
{
  "device": "ON",
  "periodic": "OFF"
}
```

## System Performance Specifications

### Data Acquisition Performance

| Metric                   | Specification          | Notes                               |
| ------------------------ | ---------------------- | ----------------------------------- |
| **Sampling Rate**        | 1 second to 60 minutes | Configurable interval               |
| **Temperature Accuracy** | ±0.2°C                 | SHT3X sensor specification          |
| **Humidity Accuracy**    | ±2% RH                 | SHT3X sensor specification          |
| **Timestamp Accuracy**   | ±2ppm (0-40°C)         | DS3231 RTC with battery backup      |
| **End-to-End Latency**   | <150ms                 | Sensor read → Web dashboard display |

### Communication Performance

| Metric                | Specification                  | Notes                                            |
| --------------------- | ------------------------------ | ------------------------------------------------ |
| **UART Baud Rate**    | 115200bps                      | STM32 ↔ ESP32, 8N1, no flow control              |
| **I2C Bus Speed**     | 100kHz                         | SHT3X + DS3231 shared bus                        |
| **SPI Clock Speed**   | ~0.28 MHz (SD), ~4.5 MHz (LCD) | Configurable via prescaler                       |
| **WiFi Standard**     | 802.11 b/g/n                   | 2.4GHz only (ESP32 limitation)                   |
| **MQTT QoS**          | 0 (data), 1 (commands)         | QoS 0 for high-throughput, QoS 1 for reliability |
| **WebSocket Latency** | <50ms                          | Web dashboard real-time updates                  |

### Storage & Buffering

| Metric               | Specification   | Notes                                |
| -------------------- | --------------- | ------------------------------------ |
| **SD Card Capacity** | 204,800 records | ~11.9 days at 5-second interval      |
| **UART Ring Buffer** | 256 bytes       | Interrupt-driven, prevents data loss |
| **MQTT Buffer**      | 8KB (ESP32)     | FreeRTOS queue-based                 |
| **Firebase Storage** | Unlimited       | Cloud historical data retention      |

### Power Consumption

| Component               | Typical Current | Peak Current  | Notes                    |
| ----------------------- | --------------- | ------------- | ------------------------ |
| **STM32F103C8T6**       | 30-40mA         | 80mA          | Active mode at 72MHz     |
| **ESP32 (WiFi active)** | 80-120mA        | 240mA         | Transmitting/receiving   |
| **SHT3X Sensor**        | <2mA            | 2mA           | During measurement       |
| **DS3231 RTC**          | 0.2mA           | 0.2mA         | Battery backup mode: 3µA |
| **SD Card**             | 20-50mA         | 100mA         | Write operations         |
| **ILI9225 Display**     | 15-20mA         | 20mA          | Active display           |
| **Relay Module**        | 15-20mA         | 70mA          | Coil energized           |
| **Total System**        | ~200mA @ 3.3V   | ~500mA @ 3.3V | All components active    |

## Project Directory Structure

```
DATALOGGER/
├── LICENSE.md                             # MIT License
├── README.md                              # This file project overview
│
├── broker/                                # MQTT Broker (Mosquitto)
│   ├── README.md                          # Broker setup & configuration guide
│   ├── mosquitto.conf                     # Main broker configuration
│   └── config/
│       └── auth/
│           └── passwd.txt                 # Bcrypt password file
│
├── documents/                             # Technical Documentation
│   ├── README.md                          # Documentation overview (110 diagrams)
│   │
│   ├── esp32/                             # ESP32 Gateway Documentation (22 diagrams)
│   │   ├── README.md                      # ESP32 documentation overview
│   │   ├── FLOW_DIAGRAM_ESP32.md          # 10 flowcharts
│   │   ├── SEQUENCE_DIAGRAM_ESP32.md      # 7 sequence diagrams
│   │   ├── UML_CLASS_DIAGRAM_ESP32.md     # 5 architecture diagrams
│   │   └── diagrams/
│   │
│   ├── firmware/                          # Integrated Firmware Documentation (23 diagrams)
│   │   ├── README.md                      # Firmware system overview
│   │   ├── FLOW_DIAGRAM_FIRMWARE.md       # 10 flowcharts
│   │   ├── SEQUENCE_DIAGRAM_FIRMWARE.md   # 10 sequence diagrams
│   │   ├── UML_DIAGRAM_FIRMWARE.md        # 3 architecture diagrams
│   │   └── diagrams/
│   │
│   ├── stm32/                             # STM32 Sensor Controller Documentation (29 diagrams)
│   │   ├── README.md                      # STM32 documentation overview
│   │   ├── FLOW_DIAGRAM_STM32.md          # 10 flowcharts
│   │   ├── SEQUENCE_DIAGRAM_STM32.md      # 8 sequence diagrams
│   │   ├── UML_CLASS_DIAGRAM_STM32.md     # 11 architecture diagrams
│   │   └── diagrams/
│   │
│   └── web/                               # Web Dashboard Documentation (36 diagrams)
│       ├── README.md                      # Web documentation overview
│       ├── FLOW_DIAGRAM_WEB.md            # 15 flowcharts
│       ├── SEQUENCE_DIAGRAM_WEB.md        # 11 sequence diagrams
│       ├── UML_DIAGRAM_WEB.md             # 10 architecture diagrams
│       └── diagrams/
│
├── firmware/                              # Embedded Firmware
│   ├── README.md                          # Firmware overview
│   │
│   ├── STM32/                             # STM32F103C8T6 Firmware
│   │   ├── README.md                      # STM32 comprehensive documentation
│   │   ├── STM32_DATALOGGER.ioc           # STM32CubeMX configuration
│   │   ├── Core/                          # HAL-generated code
│   │   ├── Datalogger_Lib/                # Custom libraries (18 modules)
│   │   │   ├── README.md                  # Library overview
│   │   │   ├── inc/                       # Library headers
│   │   │   ├── src/                       # Library implementations
│   │   │   └── docs/                      # Module-specific documentation
│   │   └── Drivers/                       # STM32 HAL drivers
│   │
│   └── ESP32/                             # ESP32 Gateway Firmware
│       ├── README.md                      # ESP32 gateway documentation
│       ├── CMakeLists.txt                 # ESP-IDF project configuration
│       ├── main/                          # Main application
│       └── components/                    # ESP-IDF Components (9 modules)
│           ├── README.md                  # Components overview
│           ├── wifi_manager/              # WiFi connection management
│           ├── mqtt_handler/              # MQTT v5 client
│           ├── stm32_uart/                # UART bridge to STM32
│           ├── relay_control/             # GPIO relay control
│           ├── json_sensor_parser/        # Parse STM32 sensor JSON
│           ├── json_utils/                # JSON utilities
│           ├── ring_buffer/               # Circular buffer for UART
│           ├── button_handler/            # Physical button input
│           └── coap_handler/              # CoAP protocol (optional)
│
└── web/                                   # Web Dashboard
    ├── README.md                          # Dashboard documentation
    ├── index.html                         # Main HTML structure
    ├── app.js                             # JavaScript application logic
    └── style.css                          # CSS styling
```

## Troubleshooting

### Common Issues

**STM32 Not Responding:**

- Check ST-Link connection (SWDIO, SWCLK, GND, 3.3V)
- Verify 5V power supply to STM32
- Check UART baud rate: must be 115200
- Press RESET button on STM32 after flashing

**ESP32 WiFi Connection Failed:**

- Verify WiFi SSID/password in `idf.py menuconfig`
- Check WiFi signal strength (move closer to router)
- Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
- Check serial monitor for error messages: `idf.py monitor`

**MQTT Connection Failed:**

- Verify broker IP address in ESP32 configuration
- Check broker is running: `docker ps | grep mqtt-broker`
- Test broker port: `telnet [broker-ip] 1883`
- Verify username/password authentication
- Check broker logs: `docker logs mqtt-broker`

**Web Dashboard Not Connecting:**

- Check WebSocket URL: must be `ws://` not `wss://`
- Verify broker port 8083 is accessible (firewall/router)
- Check browser console (F12) for JavaScript errors
- Test MQTT WebSocket: `wscat -c ws://[broker-ip]:8083`

**Sensor Reading Errors:**

- Check SHT3X I2C connections (PB6/PB7)
- Verify I2C address: default 0x44 (ADDR pin to GND)
- Check 3.3V power supply to sensor
- Ensure 4.7kΩ pull-up resistors on I2C lines

**SD Card Not Detected:**

- Verify SD card is FAT32 formatted
- Check SPI connections (PA4-PA7)
- Ensure SD card is inserted properly
- Check 3.3V power to SD card module

**ILI9225 Display Blank:**

- Verify SPI connections and CS/RS/RST GPIO pins
- Check display power supply
- Ensure correct SPI mode configuration (Mode 3)
- Test with LCD initialization sequence

For detailed troubleshooting guides, refer to component-specific README files.

## Contributing

Contributions are welcome! Please follow these guidelines:

1. **Fork the repository** and create a feature branch
2. **Follow code style** guidelines for respective components
3. **Add unit tests** for new functionality
4. **Update documentation** (README, component docs, diagrams)
5. **Test thoroughly** on actual hardware before submitting PR
6. **Submit pull request** with detailed description of changes

## Documentation References

For detailed component-specific documentation, please refer to:

### Firmware Documentation

- **Firmware Overview**: [firmware/README.md](firmware/README.md) - Complete firmware architecture and communication protocols
- **STM32 Firmware**: [firmware/STM32/README.md](firmware/STM32/README.md) - Hardware setup, command interface, JSON format, SD buffering
- **STM32 Library Modules**: [firmware/STM32/Datalogger_Lib/README.md](firmware/STM32/Datalogger_Lib/README.md) - 18 custom library modules documentation
- **ESP32 Firmware**: [firmware/ESP32/README.md](firmware/ESP32/README.md) - Component architecture, MQTT topics, WiFi configuration
- **ESP32 Components**: [firmware/ESP32/components/README.md](firmware/ESP32/components/README.md) - 9 modular components documentation

### Infrastructure Documentation

- **MQTT Broker**: [broker/README.md](broker/README.md) - Docker deployment, authentication, monitoring, and security
- **Web Dashboard**: [web/README.md](web/README.md) - Features, deployment, MQTT integration, Firebase setup

### Technical Diagrams

- **Firmware Diagrams**: [documents/firmware/](documents/firmware/) - 31 comprehensive flowcharts, sequences, and UML diagrams
- **STM32 Diagrams**: [documents/stm32/](documents/stm32/) - 15 flowcharts, 10 sequences, 11 architecture diagrams
- **ESP32 Diagrams**: [documents/esp32/](documents/esp32/) - 15 flowcharts, 10 sequences, 11 architecture diagrams
- **Web Dashboard Diagrams**: [documents/web/](documents/web/) - 15 flowcharts, 10 sequences, 11 architecture diagrams

## License

This project is licensed under the **MIT License** - see [LICENSE.md](LICENSE.md) for complete details.
