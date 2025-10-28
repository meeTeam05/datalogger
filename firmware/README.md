# Firmware

This directory contains the complete firmware implementation for the DATALOGGER system, consisting of two microcontrollers working in cooperation: STM32 for sensor acquisition and local control, and ESP32 for WiFi connectivity and cloud communication.

## System Architecture

```
                         SD Card Buffer
                              ^
                              |
Sensors (SHT3X, DS3231) --> STM32 --> UART --> ESP32 --> WiFi/MQTT --> Cloud/Web
                              |                  |
                              v                  v
                          LCD Display       Relay Control

```

The system implements a distributed architecture where:

- STM32 handles real-time sensor acquisition, display, and local storage
- ESP32 manages network connectivity and remote command processing
- Communication uses JSON format over UART at 115200 baud
- Data buffering ensures no data loss during network outages

## Directory Structure

```
firmware/
├── STM32/                      # STM32 firmware
│   ├── Core/                   # HAL and startup code
│   ├── Datalogger_Lib/         # Custom library modules
│   │   ├── inc/                # Header files
│   │   ├── src/                # Implementation files
│   │   └── README.md           # Library documentation
│   ├── Drivers/                # STM32 HAL and CMSIS
│   ├── STM32_DATALOGGER.ioc    # CubeMX configuration
│   └── README.md               # STM32 documentation
├── ESP32/                      # ESP32 firmware
│   ├── main/                   # Main application
│   ├── components/             # Modular components
│   │   ├── ring_buffer/        # Circular FIFO buffer
│   │   ├── stm32_uart/         # STM32 communication
│   │   ├── json_utils/         # JSON formatting
│   │   ├── json_sensor_parser/ # JSON parsing
│   │   ├── mqtt_handler/       # MQTT client wrapper
│   │   ├── wifi_manager/       # WiFi management
│   │   ├── coap_handler/       # CoAP protocol (optional)
│   │   ├── relay_control/      # GPIO relay control
│   │   └── button_handler/     # Button input
│   ├── CMakeLists.txt          # ESP-IDF build config
│   └── README.md               # ESP32 documentation
└── README.md                   # This file
```

## Components Overview

### STM32 Firmware

The STM32 firmware handles all sensor interfacing, local data management, display control, and offline data buffering.

**Hardware Platform**

- Microcontroller: STM32F103C8T6 (ARM Cortex-M3, 72 MHz)
- Flash Memory: 64 KB
- RAM: 20 KB
- Peripherals: I2C1, SPI1, UART1

**Key Modules**

- SHT3X Driver: I2C temperature and humidity sensor interface
- DS3231 Driver: I2C real-time clock with battery backup
- SD Card Manager: High-level data buffering (204,800 records)
- SD Card Driver: Low-level SPI block operations
- ILI9225 Display: 176x220 TFT LCD with real-time status
- Data Manager: Centralized sensor data and state management
- UART Handler: Interrupt-driven communication with ESP32
- Ring Buffer: 256-byte circular FIFO for UART reception
- Command Parser: Text-based command interpreter
- JSON Output: Standardized data formatting

**Features**

- Single-shot and periodic measurement modes
- Configurable measurement interval (1 second to 1 hour)
- Real-time clock with Unix timestamp generation
- SD card buffering for offline operation
- Automatic data synchronization when ESP32 reconnects
- LCD display showing sensor readings, time, and system status
- Sensor heater control for condensation removal
- Low-level I2C communication with CRC validation
- Command-based control interface

**Communication Interface**

- UART1: 115200 baud, 8N1, no flow control
- Protocol: JSON messages over newline-terminated text
- Pins: PA9 (TX to ESP32), PA10 (RX from ESP32)

### ESP32 Firmware

The ESP32 firmware provides WiFi connectivity, MQTT communication, and remote device control.

**Hardware Platform**

- Module: ESP32-WROOM-32 or compatible
- Flash Memory: 4 MB
- RAM: 520 KB
- Connectivity: WiFi 802.11 b/g/n, Bluetooth (not used)

**Key Components**

- WiFi Manager: Connection management with auto-reconnect
- MQTT Handler: MQTT v5.0 client with QoS support
- STM32 UART: Communication with sensor controller
- JSON Parser: Sensor data extraction from STM32 messages
- JSON Utils: Standardized message formatting
- Relay Control: GPIO-based relay switching
- Ring Buffer: UART receive buffering
- CoAP Handler: Lightweight protocol alternative (optional)
- Button Handler: Physical button input with debouncing

**Features**

- WiFi connection with configurable credentials
- MQTT broker communication with TLS support
- Automatic reconnection on network failures
- Command forwarding from MQTT to STM32
- Sensor data publishing to MQTT topics
- Relay control via MQTT commands
- JSON protocol for all data exchange
- Configurable power save modes
- Event-driven architecture

**Communication Interface**

- UART2: 115200 baud, 8N1, no flow control
- Protocol: JSON messages over newline-terminated text
- Pins: GPIO16 (RX from STM32), GPIO17 (TX to STM32)
- MQTT: Broker connection over WiFi
- Default Topics:
  - datalogger/esp32/command
  - datalogger/esp32/relay/control
  - datalogger/esp32/system/state
  - datalogger/stm32/single/data
  - datalogger/stm32/periodic/data

## Data Flow Architecture

### Normal Operation (MQTT Connected)

```
1. Sensor Reading
   SHT3X --> I2C --> STM32

2. Timestamp Addition
   DS3231 --> I2C --> STM32 (Unix timestamp)

3. JSON Formatting
   STM32 Data Manager --> JSON String
   Example: {"mode":"SINGLE","timestamp":1729699200,"temperature":25.50,"humidity":60.00}
   Example: {"mode":"PERIODIC","timestamp":1729699205,"temperature":25.55,"humidity":60.05}

4. UART Transmission
   STM32 UART --> Ring Buffer --> ESP32 UART

5. JSON Parsing
   ESP32 JSON Parser --> Extract fields

6. MQTT Publishing
   ESP32 --> WiFi --> MQTT Broker --> Web Dashboard

7. Display Update
   STM32 --> ILI9225 LCD (real-time status)
```

### Offline Operation (MQTT Disconnected)

```
1. Sensor Reading
   SHT3X --> I2C --> STM32

2. Timestamp Addition
   DS3231 --> I2C --> STM32

3. SD Card Buffering
   STM32 --> SPI --> SD Card (512-byte blocks)
   Circular buffer: 204,800 records capacity

4. Display Update
   STM32 --> ILI9225 LCD (shows buffered count)

5. When MQTT Reconnects
   STM32 receives "MQTT CONNECTED" notification form ESP32
   --> Reads buffered data from SD card
   --> Transmits via UART to ESP32
   --> ESP32 publishes to MQTT
   --> STM32 removes transmitted records
```

### Command Flow (Remote to Sensor)

```
1. Web Dashboard
   User clicks "Start Periodic" button

2. MQTT Publish
   Web --> MQTT Broker
   Topic: datalogger/esp32/command
   Payload: "PERIODIC ON"

3. ESP32 Subscribe
   MQTT Handler receives message

4. UART Forward
   ESP32 --> UART --> STM32
   Message: "PERIODIC ON\r\n"

5. Command Execution
   STM32 Command Parser --> Data Manager --> SHT3X Driver
   Start periodic measurements

6. Continuous Data Flow
   Periodic sensor readings --> JSON --> UART --> ESP32 --> MQTT --> Web
```

## Communication Protocol

### JSON Message Format

All communication between STM32 and ESP32 uses JSON format with newline termination.

**Sensor Data Message (STM32 to ESP32)**

```json
{
  "mode": "SINGLE",
  "timestamp": 1729699200,
  "temperature": 25.5,
  "humidity": 60.0
}
```

Fields:

- mode: "SINGLE" or "PERIODIC"
- timestamp: Unix timestamp (seconds since epoch)
- temperature: Float, degrees Celsius, range -40 to 125
- humidity: Float, percent RH, range 0 to 100

**Command Message (ESP32 to STM32)**

```
SINGLE\r\n
PERIODIC ON\r\n
PERIODIC OFF\r\n
SET PERIODIC INTERVAL 5000\r\n
SET TIME 1760000000\r\n
MQTT CONNECTED\r\n
MQTT DISCONNECTED\r\n
```

### MQTT Topics

**Publishing (ESP32 → Broker)**

Topics that ESP32 publishes data to:

```
datalogger/stm32/single/data              # Single sensor reading (JSON)
datalogger/stm32/periodic/data            # Periodic sensor sensor reading (JSON)
datalogger/esp32/system/state             # ESP32 system status
datalogger/esp32/relay/control            # Relay state feedback (ON/OFF)
```

**Subscribing (Broker → ESP32)**

Topics that ESP32 listens to for incoming commands:

```
datalogger/stm32/command                  # STM32 sensor control commands
datalogger/esp32/relay/control            # Relay control commands (ON/OFF)
```

### Command Reference

| Command               | Description                      | Example                     |
| --------------------- | -------------------------------- | --------------------------- |
| SINGLE                | Trigger single measurement       | SINGLE                      |
| PERIODIC ON           | Start periodic measurements      | PERIODIC ON                 |
| PERIODIC OFF          | Stop periodic measurements       | PERIODIC OFF                |
| SET PERIODIC INTERVAL | Set interval in milliseconds     | SET PERIODIC INTERVAL 10000 |
| SET TIME              | Set RTC date/time                | SET TIME 1760000000         |
| MQTT CONNECTED        | ESP32 connection notification    | MQTT CONNECTED              |
| MQTT DISCONNECTED     | ESP32 disconnection notification | MQTT DISCONNECTED           |

## Hardware Setup

### STM32 Connections

**I2C1 Devices**

| Device     | Pin | Connection           |
| ---------- | --- | -------------------- |
| SHT3X SCL  | PB6 | 4.7k pull-up to 3.3V |
| SHT3X SDA  | PB7 | 4.7k pull-up to 3.3V |
| DS3231 SCL | PB6 | Shared with SHT3X    |
| DS3231 SDA | PB7 | Shared with SHT3X    |

**SPI1 Devices**

| Device       | Pin  | Connection                    |
| ------------ | ---- | ----------------------------- |
| SD Card CS   | PA4  | GPIO output                   |
| SD Card CLK  | PA5  | SPI clock                     |
| SD Card MISO | PA6  | SD card data out              |
| SD Card MOSI | PA7  | SPI data                      |
| ILI9225 CS   | PB12 | GPIO output                   |
| ILI9225 SCK  | PB13 | SPI clock                     |
| ILI9225 SDA  | PB15 | SPI data                      |
| ILI9225 RS   | PA11 | GPIO output (register select) |
| ILI9225 RST  | PA8  | GPIO output (reset)           |

**UART1 Communication**

| Signal   | STM32 Pin | ESP32 Pin                  |
| -------- | --------- | -------------------------- |
| STM32 TX | PA9       | CONFIG_MQTT_UART_RXD (RX2) |
| STM32 RX | PA10      | CONFIG_MQTT_UART_TXD (TX2) |
| GND      | GND       | GND                        |

**Power Supply**

- VCC: 3.3V for all modules
- GND: Common ground for all modules

### ESP32 Connections

**UART2 Communication**

| Signal   | ESP32 Pin            | STM32 Pin  |
| -------- | -------------------- | ---------- |
| ESP32 TX | CONFIG_MQTT_UART_TXD | PA10 (RX1) |
| ESP32 RX | CONFIG_MQTT_UART_RXD | PA9 (TX1)  |
| GND      | GND                  | GND        |

**Relay Control**

| Signal        | ESP32 Pin             | Relay Module    |
| ------------- | --------------------- | --------------- |
| Relay Control | CONFIG_RELAY_GPIO_NUM | Signal input    |
| VCC           | 3.3V                  | VCC (if needed) |
| GND           | GND                   | GND             |

**Control Buttons**

| Button Function     | ESP32 Pin                   | Configuration |
| ------------------- | --------------------------- | ------------- |
| Relay Toggle        | CONFIG_BUTTON_RELAY_GPIO    | Pull-up       |
| Single Read         | CONFIG_BUTTON_SINGLE_GPIO   | Pull-up       |
| Periodic Read       | CONFIG_BUTTON_PERIODIC_GPIO | Pull-up       |
| Interval Adjustment | CONFIG_BUTTON_INTERVAL_GPIO | Pull-up       |

**LED Indicators**

| LED Function | ESP32 Pin            | Active Level            | Indication                    |
| ------------ | -------------------- | ----------------------- | ----------------------------- |
| WiFi Status  | CONFIG_WIFI_LED_GPIO | CONFIG_LED_ACTIVE_LEVEL | WiFi connection status        |
| MQTT Status  | CONFIG_MQTT_LED_GPIO | CONFIG_LED_ACTIVE_LEVEL | MQTT broker connection status |

### Power Requirements

| Component        | Voltage | Current    | Notes                    |
| ---------------- | ------- | ---------- | ------------------------ |
| STM32F103C8T6    | 3.3V    | 30-40 mA   | Active mode              |
| SHT3X Sensor     | 3.3V    | 1.5 mA     | During measurement       |
| DS3231 RTC       | 3.3V    | 0.2 mA     | With battery backup      |
| SD Card          | 3.3V    | 20-100 mA  | During write operations  |
| ILI9225 Display  | 3.3V    | 15-20 mA   | Active display           |
| ESP32            | 3.3V    | 80-160 mA  | WiFi active, 240 mA peak |
| Relay Module     | 3.3V    | 50-100 mA  | When energized           |
| **Total System** | 3.3V    | 200-400 mA | Typical operation        |

Recommended power supply: 3.3V voltage regulator (AMS1117-3.3 or similar)

## Building and Flashing

### STM32 Firmware

**Requirements**

- STM32CubeIDE (recommended) or ARM GCC toolchain
- ST-Link V2 programmer
- STM32CubeMX for peripheral configuration

**Build Steps**

Using STM32CubeIDE:

```bash
# Open project
File -> Open Projects from File System
Select: firmware/STM32

# Build
Project -> Build All (Ctrl+B)

# Flash
Run -> Debug (F11)
```

Using GCC toolchain:

```bash
cd firmware/STM32
make clean
make all
st-flash write build/STM32_DATALOGGER.bin 0x8000000
```

**Configuration**

```bash
# Open STM32CubeMX
open STM32_DATALOGGER.ioc

# Configure peripherals:
# - I2C1: 100 kHz, 7-bit addressing
# - SPI1: Full duplex master, prescaler 256, mode 0
# - SPI2: Full duplex master, prescaler 8, mode 3
# - UART1: 115200 baud, 8N1

# Generate code
Project -> Generate Code
```

### ESP32 Firmware

**Requirements**

- ESP-IDF v4.4 or later
- Python 3.7 or later
- USB-to-Serial driver for ESP32

**Build Steps**

```bash
cd firmware/ESP32

# Configure project
idf.py menuconfig

# Navigate through menus:
# - WiFi Connection Configuration
#   - Set SSID and password
# - IoT Protocol Configuration -> MQTT Protocol Configuration
#   - Set broker URI
#   - Set client ID
# - IoT Protocol Configuration -> CoAP Handler Configuration
#   - CoAP Sever IP Address
#   - CoAP Sever Port
# - Hardware Pin Configuration -> STM32 Communication (UART)
#   - UART port number for STM32 communication
#   - STM32 UART TXD pin number
#   - STM32 UART RXD pin number
# - Hardware Pin Configuration -> Relay Control Configuration
#   - Relay control GPIO pin number
# - Hardware Pin Configuration -> Button Handler Configuration
#   - Relay button GPIO pin number
#   - Single command button GPIO pin number
#   - Periodic toggle button GPIO pin number
#   - Interval cycle button GPIO pin number
# - Hardware Pin Configuration -> Status LED Configuration
#   - WiFi status LED GPIO pin number
#   - MQTT status LED GPIO pin number

# Build firmware
idf.py build

# Flash to ESP32
idf.py -p /dev/ttyUSB0 flash    # Linux/Mac
idf.py -p COM3 flash             # Windows

# Monitor output
idf.py monitor

# Or do all at once
idf.py -p /dev/ttyUSB0 flash monitor
```

**Configuration Options**

WiFi settings (menuconfig):

```
WiFi Connection Configuration
  WiFi SSID: "your_network"
  WiFi Password: "your_password"
  Maximum Retry: 5
```

MQTT settings (menuconfig):

```
IoT Protocol Configuration -> MQTT Protocol Configuration
  Broker URI: "mqtt://192.168.1.100:1883"
  Client ID: "datalogger_esp32"
  Username: "" (optional)
  Password: "" (optional)
```

## Testing and Verification

### STM32 Testing

**Serial Terminal Test**

```bash
# Linux/Mac
screen /dev/ttyUSB0 115200

# Windows - use PuTTY or Tera Term
# Port: COM3, Baud: 115200, 8N1

# Send commands:
CHECK UART
SINGLE
SET TIME 1760000000
PERIODIC ON
```

**I2C Device Detection**

```c
// Add to main.c temporarily
for (uint8_t addr = 0x00; addr < 0x7F; addr++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 100) == HAL_OK) {
        printf("Found I2C device at 0x%02X\n", addr);
    }
}
// Should find: 0x44 (SHT3X), 0x68 (DS3231)
```

**SD Card Test**

```bash
# Send command via serial
SD CLEAR
SINGLE
# Check if data is buffered
# Remove SD card, format FAT32, reinsert
# Send SINGLE again - should buffer to SD
```

### ESP32 Testing

**WiFi Connection Test**

```bash
idf.py monitor

# Look for:
# "WiFi connected"
# "IP Address: 192.168.1.xxx"
```

**MQTT Connection Test**

```bash
# In another terminal
mosquitto_sub -v -t "datalogger/#"

# Should see periodic messages when STM32 sends data
```

**Command Test**

```bash
# Publish command
mosquitto_pub -t "datalogger/esp32/command" -m "SINGLE"

# Monitor STM32 serial output
# Should see single measurement result
```

**Relay Test**

```bash
# Turn on relay
mosquitto_pub -t "datalogger/esp32/relay" -m "RELAY ON"

# Turn off relay
mosquitto_pub -t "datalogger/esp32/relay" -m "RELAY OFF"

# Check GPIO4 with multimeter or LED
```

### System Integration Test

**End-to-End Test**

```bash
# 1. Power up system
# 2. Wait for ESP32 WiFi connection
# 3. Start MQTT subscriber
mosquitto_sub -v -t "datalogger/#"

# 4. Send periodic start command
mosquitto_pub -t "datalogger/esp32/command" -m "PERIODIC ON"

# 5. Verify sensor data publishing every 5 seconds
# 6. Check LCD display shows current readings
# 7. Disconnect WiFi router
# 8. Wait 30 seconds (STM32 buffering to SD)
# 9. Reconnect WiFi router
# 10. Verify buffered data transmission
```

## Performance Characteristics

### Timing

| Operation          | STM32     | ESP32    | End-to-End |
| ------------------ | --------- | -------- | ---------- |
| Sensor measurement | 15-30 ms  | -        | -          |
| I2C read operation | 2-5 ms    | -        | -          |
| SD card write      | 5-10 ms   | -        | -          |
| Display update     | 50-100 ms | -        | -          |
| UART transmission  | ~8 ms     | ~8 ms    | 16 ms      |
| JSON parsing       | -         | 1-2 ms   | -          |
| MQTT publish       | -         | 10-50 ms | -          |
| Command execution  | 50-100 ms | -        | -          |
| **Total latency**  | -         | -        | 100-200 ms |

### Memory Usage

**STM32F103C8T6**
| Component | Flash | RAM |
|-----------|-------|-----|
| HAL Library | ~20 KB | ~1 KB |
| Sensor Drivers | ~4 KB | ~100 bytes |
| SD Card System | ~6 KB | ~300 bytes |
| Display System | ~8 KB | ~500 bytes |
| Communication | ~5 KB | ~300 bytes |
| Ring Buffer | - | 256 bytes |
| Application | ~3 KB | ~1 KB |
| **Total** | ~46 KB / 64 KB | ~3.5 KB / 20 KB |

**ESP32**
| Component | Flash | RAM |
|-----------|-------|-----|
| ESP-IDF Core | ~400 KB | ~40 KB |
| WiFi Stack | ~200 KB | ~60 KB |
| MQTT Handler | ~50 KB | ~8 KB |
| Custom Components | ~100 KB | ~10 KB |
| Application | ~50 KB | ~5 KB |
| **Total** | ~800 KB / 4 MB | ~123 KB / 520 KB |

### Data Rates

| Metric                            | Value              | Notes                       |
| --------------------------------- | ------------------ | --------------------------- |
| Maximum measurement rate          | 10 Hz              | Sensor hardware limit       |
| Typical periodic rate             | 0.2 Hz (5 seconds) | Default configuration       |
| UART bandwidth                    | ~11.5 KB/s         | 115200 baud theoretical     |
| Actual JSON throughput            | ~1 KB/s            | Including protocol overhead |
| MQTT message rate                 | ~10 msg/s          | Network dependent           |
| SD card write speed               | ~100 KB/s          | Class 4 card minimum        |
| SD buffer capacity                | 204,800 records    | ~100 MB at 512 bytes/record |
| Buffering duration at 5s interval | ~11.9 days         | Before buffer full          |

### Power Consumption

| Mode                      | STM32 | ESP32  | Display | Total  |
| ------------------------- | ----- | ------ | ------- | ------ |
| Active measurement        | 35 mA | 120 mA | 20 mA   | 175 mA |
| Idle (display on)         | 30 mA | 80 mA  | 15 mA   | 125 mA |
| WiFi transmission         | 30 mA | 180 mA | 20 mA   | 230 mA |
| SD card write             | 50 mA | 80 mA  | 20 mA   | 150 mA |
| Peak (WiFi TX + SD write) | 50 mA | 240 mA | 20 mA   | 310 mA |

Average power at 5-second periodic mode: ~150-200 mA @ 3.3V = 0.5-0.66 W

## Troubleshooting

### STM32 Issues

**No sensor data**

- Check I2C connections (PB6, PB7)
- Verify pull-up resistors (4.7k to 3.3V)
- Test I2C device detection code
- Check SHT3X ADDR pin connected to GND

**SD card not detected**

- Verify SPI connections (PA4, PA5, PA6, PA7)
- Format card as FAT32 or FAT16
- Use Class 4 or better microSD card
- Check 3.3V power supply stable

**Display not working**

- Verify SPI connections (PB12, PB13, PB14, PB15)
- Check RS pin (PA11) and RST pin (PA8)
- Verify backlight power
- Test with known-good display module

**UART communication failure**

- Check cross-connection (PA9 to ESP32 RX, PA10 to ESP32 TX)
- Verify baud rate 115200 in both devices
- Ensure common ground connection
- Test with loopback (PA9 to PA10)

**RTC time incorrect**

- Set time using SET TIME command
- Check CR2032 battery voltage (should be >2.8V)
- Verify DS3231 I2C address (0x68)

### ESP32 Issues

**WiFi not connecting**

- Check SSID and password in menuconfig
- Verify WiFi router is 2.4 GHz (ESP32 doesn't support 5 GHz)
- Check signal strength (keep ESP32 within range)
- Disable MAC address filtering on router

**MQTT connection fails**

- Verify broker IP address and port
- Check broker is running (docker ps)
- Test broker with mosquitto_sub
- Verify network firewall settings

**No data from STM32**

- Check UART connections (GPIO16, GPIO17, GND)
- Monitor serial output with idf.py monitor
- Verify STM32 is powered and running
- Test UART with CHECK UART command

**Relay not switching**

- Check GPIO18 connection to relay module
- Verify relay module power (may need 3.3V or 5V)
- Test GPIO with multimeter (should toggle 0V/3.3V)
- Check relay module signal voltage requirements

### System Issues

**Data loss during network outage**

- Verify SD card is inserted and formatted
- Check SD card write operations
- Monitor SD buffer count on display
- Ensure sufficient SD card space

**Display shows incorrect data**

- Check MQTT CONNECTED/DISCONNECTED notifications
- Verify RTC time is set correctly
- Monitor serial output for JSON format errors

**High latency**

- Check WiFi signal strength
- Reduce MQTT message size
- Optimize periodic interval
- Check broker network latency

## Documentation References

- STM32 Firmware Details: [STM32/README.md](STM32/README.md)
- ESP32 Firmware Details: [ESP32/README.md](ESP32/README.md)
- STM32 Datalogger Library: [STM32/Datalogger_Lib/README.md](STM32/Datalogger_Lib/README.md)
- ESP32 Components: [ESP32/components/README.md](ESP32/components/README.md)
- Firmware Architecture: [../documents/firmware/](../documents/firmware/)
- System Architecture: [../documents/SYSTEM/](../documents/SYSTEM/)
- Web Dashboard: [../web/README.md](../web/README.md)

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
