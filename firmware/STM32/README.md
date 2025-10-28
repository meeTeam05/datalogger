# STM32 Firmware

This directory contains the firmware for the STM32F103C8T6 microcontroller in the DATALOGGER system. It handles data acquisition from SHT3X temperature/humidity sensors, DS3231 real-time clock, SD card data buffering, and ILI9225 LCD display. The firmware provides a command-line interface via UART for communication with the ESP32 module, supporting sensor control, measurement configuration, and JSON-formatted data output.

## Firmware Architecture

The firmware implements an interrupt-driven UART command processor with centralized data management, SD card buffering, and real-time display:

```
                    ┌──────────────────────┐
                    │  UART RX Interrupt   │
                    └──────────┬───────────┘
                               │
                               ↓
                    ┌──────────────────────┐
                    │     Ring Buffer      │
                    └──────────┬───────────┘
                               │
                               ↓
                    ┌──────────────────────┐
                    │    Command Parser    │
                    └──────────┬───────────┘
                               │
                               ↓
                    ┌──────────────────────┐
                    │  I2C Sensor Drivers  │
                    └──────────┬───────────┘
                               │
                               ↓
                    ┌──────────────────────┐
                    │     Data Manager     │
                    └──────────┬───────────┘
                               │
                               ↓
                    ┌──────────────────────┐
                    │  JSON Formatter +    │
                    │   RTC Timestamp      │
                    └──────────┬───────────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
              ↓                ↓                ↓
     ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
     │              │  │  SD Card     │  │   ILI9225    │
     │   UART TX    │  │   Buffer     │  │   Display    │
     │  (to ESP32)  │  │  (offline    │  │  (real-time  │
     │              │  │   storage)   │  │   status)    │
     └──────────────┘  └──────────────┘  └──────────────┘
```

Command flow: Commands received from ESP32 via UART are buffered, parsed, dispatched to sensor drivers. Results are formatted as JSON with Unix timestamps from DS3231 RTC and sent to ESP32. If ESP32 is offline, data is buffered to SD card for later transmission. Display shows real-time sensor readings and system status.

## Directory Structure

```
STM32/
├── Core/                               # Main STM32CubeMX-generated code
│   ├── Inc/
│   │   └── main.h                      # Main application header
│   ├── Src/
│   │   ├── main.c                      # Application entry point and main loop
│   │   ├── stm32f1xx_it.c              # Interrupt service routines
│   │   └── stm32f1xx_hal_msp.c         # HAL MSP initialization callbacks
│   └── Startup/
│       └── startup_stm32f103c8tx.s     # Startup assembly code
│
├── Datalogger_Lib/                     # Custom firmware library modules
│   ├── docs/
│   │   ├── cmd_execute.md              # Command execution documentation
│   │   ├── cmd_func.md                 # Command functions documentation
│   │   ├── cmd_parser.md               # Command parser documentation
│   │   ├── data_manager.md             # Data manager documentation
│   │   ├── display.md                  # Display controller documentation
│   │   ├── ds3231.md                   # DS3231 RTC driver documentation
│   │   ├── fonts.md                    # Font system documentation
│   │   ├── ili9225.md                  # ILI9225 LCD driver documentation
│   │   ├── print_cli.md                # CLI printing documentation
│   │   ├── ring_buffer.md              # Ring buffer documentation
│   │   ├── sd_card_manager.md          # SD card manager documentation
│   │   ├── sd_card.md                  # SD card driver documentation
│   │   ├── sensor_json_output.md       # JSON output documentation
│   │   ├── sht3x.md                    # SHT3X sensor driver documentation
│   │   ├── uart.md                     # UART handler documentation
│   │   └── wifi_manager.md             # Wi-Fi / MQTT manager documentation
│   ├── inc/
│   │   ├── cmd_execute.h               # Command dispatcher
│   │   ├── cmd_func.h                  # Command table declarations
│   │   ├── cmd_parser.h                # Command parsing functions
│   │   ├── data_manager.h              # Data management module
│   │   ├── display.h                   # Display controller
│   │   ├── ds3231.h                    # DS3231 RTC driver interface
│   │   ├── fonts.h                     # Font definitions
│   │   ├── ili9225.h                   # ILI9225 LCD driver
│   │   ├── print_cli.h                 # Formatted UART output functions
│   │   ├── ring_buffer.h               # Circular buffer implementation
│   │   ├── sd_card_manager.h           # High-level SD buffering
│   │   ├── sd_card.h                   # Low-level SD card driver (SPI)
│   │   ├── sensor_json_output.h        # JSON formatter
│   │   ├── sht3x.h                     # SHT3X sensor driver interface
│   │   ├── uart.h                      # UART communication with ring buffer
│   │   └── wifi_manager.h              # ESP32 Wi-Fi status tracking
│   ├── src/
│   │   ├── cmd_execute.c               # Command tokenizer and executors
│   │   ├── cmd_func.c                  # Command table definition
│   │   ├── cmd_parser.c                # Command handler implementations
│   │   ├── data_manager.c              # Centralized data state management
│   │   ├── display.c                   # Display update logic
│   │   ├── ds3231.c                    # DS3231 I2C communication
│   │   ├── fonts.c                     # Font data
│   │   ├── ili9225.c                   # ILI9225 hardware driver
│   │   ├── print_cli.c                 # UART transmission functions
│   │   ├── ring_buffer.c               # Ring buffer operations
│   │   ├── sd_card_manager.c           # SD card buffering and metadata
│   │   ├── sd_card.c                   # SD card SPI protocol implementation
│   │   ├── sensor_json_output.c        # JSON serialization functions
│   │   ├── sht3x.c                     # SHT3X I2C communication
│   │   ├── uart.c                      # UART interrupt handler and buffer management
│   │   └── wifi_manager.c              # Wi-Fi status tracking
│   └── README.md                       # Library overview and integration guide
│
├── Drivers/                            # STM32 peripheral drivers
│   ├── STM32F1xx_HAL_Driver/           # STM32F1 HAL library
│   └── CMSIS/                          # ARM CMSIS standard headers
│
├── STM32_DATALOGGER.ioc                # STM32CubeMX configuration file
├── STM32F103C8TX_FLASH.ld              # Linker script for STM32F103C8T6
└── README.md                           # Project root documentation

```

Note: The Drivers directory contains STM32 HAL peripheral drivers and CMSIS headers generated by STM32CubeMX.

## Key Features and Capabilities

- Interrupt-driven UART reception with 256-byte ring buffer for reliable command processing
- String-based command dispatch system with exact matching for sensor control
- Dual measurement modes: single-shot on-demand and periodic continuous sampling
- JSON-formatted output with Unix timestamps from DS3231 real-time clock
- I2C communication at 100 kHz with CRC validation for sensor data integrity
- Centralized data management architecture for consistent output formatting
- Support for SHT3X sensor measurement rates from 0.5 Hz to 10 Hz
- Built-in heater control for condensation prevention
- SD card data buffering for offline operation (204,800 records capacity)
- Persistent storage with automatic overflow handling and sequence numbering
- Real-time LCD display (176x220 ILI9225) showing sensor readings and system status
- ESP32 connectivity status tracking (MQTT connected/disconnected notifications)
- Configurable periodic measurement interval via command interface
- Low latency command-to-response time under 100 milliseconds
- Automatic data synchronization when ESP32 reconnects

## Hardware Requirements

### Components

- STM32F103C8T6 microcontroller or compatible STM32F1 series
- SHT30, SHT31, or SHT35 temperature and humidity sensor
- DS3231 real-time clock module with CR2032 battery backup
- MicroSD card (FAT32 formatted, 1 GB to 32 GB)
- ILI9225 TFT LCD display (176x220 pixels, SPI interface)
- ST-Link V2 or compatible programmer for firmware flashing
- ESP32 module for WiFi/MQTT communication
- Two 4.7 kΩ pull-up resistors for I2C bus (if not included in sensor modules)

### Pin Connections

| STM32 Pin | Function      | Connected To              | Notes                                   |
| --------- | ------------- | ------------------------- | --------------------------------------- |
| PB6       | I2C1 SCL      | SHT3X SCL, DS3231 SCL     | Shared I2C clock line, requires pull-up |
| PB7       | I2C1 SDA      | SHT3X SDA, DS3231 SDA     | Shared I2C data line, requires pull-up  |
| PA9       | UART1 TX      | ESP32 GPIO16 (RX)         | Command responses and data output       |
| PA10      | UART1 RX      | ESP32 GPIO17 (TX)         | Command input from ESP32                |
| PA4       | SPI1 CS (SD)  | SD Card CS                | SD card chip select                     |
| PA5       | SPI1 SCK      | SD Card CLK, ILI9225 SCK  | SPI clock                               |
| PA6       | SPI1 MISO     | SD Card MISO              | SD card data out                        |
| PA7       | SPI1 MOSI     | SD Card MOSI, ILI9225 SDA | SPI data                                |
| PB12      | SPI1 CS (LCD) | ILI9225 CS                | Display chip select                     |
| PB13      | SPI1 SCK      | ILI9225 SCK               | SPI clock                               |
| PB15      | SPI1 MOSI     | ILI9225 SDA               | SPI data                                |
| PA11      | GPIO OUT      | ILI9225 RS                | Display register select                 |
| PA8       | GPIO OUT      | ILI9225 RST               | Display reset                           |
| 3.3V      | Power supply  | All modules VCC           | Common power rail                       |
| GND       | Ground        | All modules GND           | Common ground reference                 |

### I2C Device Addresses

| Device | ADDR Pin Connection | 7-bit I2C Address | Default Configuration |
| ------ | ------------------- | ----------------- | --------------------- |
| SHT3X  | Connected to GND    | 0x44              | Used in firmware      |
| SHT3X  | Connected to VDD    | 0x45              | Alternative address   |
| DS3231 | Not applicable      | 0x68              | Fixed address         |

Note: The SHT3X sensor ADDR pin must be connected to GND for the default firmware configuration (address 0x44).

### SPI Device Configuration

| Device          | SPI Port | CS Pin | Configured Clock            | Max Supported | SPI Mode |
| --------------- | -------- | ------ | --------------------------- | ------------- | -------- |
| SD Card         | SPI1     | PA4    | ~0.28 MHz (Prescaler = 256) | Up to 18 MHz  | Mode 0   |
| ILI9225 Display | SPI2     | PB0    | ~4.5 MHz (Prescaler = 8)    | Up to 36 MHz  | Mode 3   |

## Software Development Environment

### Required Tools

- STM32CubeIDE (recommended), Keil MDK, or GCC ARM toolchain
- STM32CubeMX for peripheral configuration
- ST-Link Utility or OpenOCD for firmware flashing
- Serial terminal application: PuTTY, Tera Term, minicom, or screen

### Building the Firmware

#### Method 1: STM32CubeIDE

1. Import the project:

   - File → Open Projects from File System
   - Select the STM32 directory

2. Build the project:

   - Project → Build All or press Ctrl+B

3. Flash the firmware:
   - Run → Debug or press F11

#### Method 2: GCC ARM Toolchain

Install the toolchain:

- Windows: Download from ARM website (https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
- Ubuntu/Debian: `sudo apt-get install gcc-arm-none-eabi openocd`
- macOS: `brew install gcc-arm-embedded openocd`

Build and flash:

```bash
cd STM32/
make clean
make all
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/STM32_DATALOGGER.elf verify reset exit"
```

### Serial Terminal Configuration

#### Windows - PuTTY

Connection settings:

- Connection Type: Serial
- Serial Line: COM3 (check Device Manager for actual port)
- Speed: 115200
- Data bits: 8
- Stop bits: 1
- Parity: None
- Flow control: None

#### Linux/macOS - minicom or screen

Identify the serial port:

```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

Connect using minicom:

```bash
minicom -D /dev/ttyUSB0 -b 115200
```

Or using screen:

```bash
screen /dev/ttyUSB0 115200
```

#### Python - pySerial

```python
import serial

ser = serial.Serial(
    port='/dev/ttyUSB0',  # Windows: 'COM3'
    baudrate=115200,
    bytesize=8,
    parity='N',
    stopbits=1,
    timeout=1
)

ser.write(b"SHT3X SINGLE HIGH\n")
response = ser.readline()
print(response.decode())
```

## Command Interface

The firmware receives commands from the ESP32 module via UART. Commands are newline-terminated strings.

### Measurement Commands

#### Single Measurement

Trigger one immediate sensor measurement and send JSON output.

Command format:

```
SINGLE\r\n
```

Example response:

```json
{
  "mode": "SINGLE",
  "timestamp": 1729699200,
  "temperature": 25.5,
  "humidity": 60.0
}
```

The measurement is performed immediately, formatted with RTC timestamp, and transmitted to ESP32. If ESP32 is offline (no MQTT connection), the data is automatically buffered to SD card.

#### Periodic Measurement Control

Start or stop continuous periodic measurements.

Commands:

```
PERIODIC ON\r\n              # Start periodic measurements
PERIODIC OFF\r\n             # Stop periodic measurements
```

When periodic mode is active, measurements are taken at the configured interval (default 5 seconds) and automatically transmitted to ESP32 or buffered to SD card if offline.

Example periodic output:

```json
{"mode":"PERIODIC","timestamp":1729699200,"temperature":25.50,"humidity":60.00}
{"mode":"PERIODIC","timestamp":1729699205,"temperature":25.51,"humidity":60.02}
{"mode":"PERIODIC","timestamp":1729699210,"temperature":25.49,"humidity":59.98}
```

### Configuration Commands

#### Set Periodic Interval

Configure the time interval between periodic measurements.

Command format:

```
SET PERIODIC INTERVAL <milliseconds>\r\n
```

Example:

```
SET PERIODIC INTERVAL 10000\r\n    # Set to 10 seconds
SET PERIODIC INTERVAL 5000\r\n     # Set to 5 seconds (default)
SET PERIODIC INTERVAL 1000\r\n     # Set to 1 second
```

Valid range: 1000 ms (1 second) to 3600000 ms (1 hour)

#### Set RTC Time

Configure the DS3231 real-time clock.

##### Option A:

Command format:

```
SET TIME <YYYY> <MM> <DD> <HH> <MM> <SS>\r\n
```

Example:

```
SET TIME 2025 10 23 14 30 00\r\n
```

Parameters:

- YYYY: Year (2000-2099)
- MM: Month (1-12)
- DD: Day (1-31)
- HH: Hour (0-23)
- MM: Minute (0-59)
- SS: Second (0-59)

##### Option B:

```
SET TIME <timestamp>\r\n
```

Example:

```
SET TIME 1760000000\r\n
```

Parameters:

- timestamp: Unix timestamp (seconds since 00:00:00 UTC, January 1, 1970)

### Sensor Control Commands

#### Heater Control

Enable or disable the SHT3X integrated heater for condensation removal.

Commands:

```
SHT3X HEATER ENABLE\r\n      # Turn on heater
SHT3X HEATER DISABLE\r\n     # Turn off heater
```

Note: The heater draws approximately 5.5 mW and increases sensor temperature. Disable before taking measurements.

#### Accelerated Response Time

Run the SHT3X self-test to verify sensor functionality.

Command:

```
SHT3X ART\r\n
```

This command performs the Accelerated Response Time test, which quickly heats and cools the sensor to verify proper operation.

### System Commands

#### WiFi Status Notifications

The ESP32 module sends notifications to STM32 when MQTT connection state changes.

Commands:

```
MQTT CONNECTED\r\n           # ESP32 is connected to MQTT broker
MQTT DISCONNECTED\r\n        # ESP32 lost MQTT connection
```

The STM32 uses these notifications to determine whether to send data directly to ESP32 or buffer it to SD card.

#### SD Card Management

Clear all buffered data from the SD card.

Command:

```
SD CLEAR\r\n
```

This removes all stored sensor readings from the SD card buffer. Use with caution - data cannot be recovered after clearing.

#### UART Status Check

Verify UART communication is working.

Command:

```
CHECK UART\r\n
```

Response:

```
UART OK
```

### Command Summary

| Command                 | Description                   | Response         |
| ----------------------- | ----------------------------- | ---------------- |
| SINGLE                  | Trigger single measurement    | JSON data        |
| PERIODIC ON             | Start periodic measurements   | JSON data stream |
| PERIODIC OFF            | Stop periodic measurements    | None             |
| SET PERIODIC INTERVAL n | Set measurement interval (ms) | None             |
| SET TIME Y M D H M S    | Set RTC date/time             | None             |
| SET TIME `<timestamp>`  | Set RTC date/time             | None             |
| SHT3X HEATER ENABLE     | Enable sensor heater          | None             |
| SHT3X HEATER DISABLE    | Disable sensor heater         | None             |
| SHT3X ART               | Run sensor self-test          | None             |
| MQTT CONNECTED          | ESP32 status notification     | None             |
| MQTT DISCONNECTED       | ESP32 status notification     | None             |
| SD CLEAR                | Erase all buffered data       | None             |
| CHECK UART              | UART communication test       | UART OK          |

## Data Output Format

All sensor measurements are transmitted as JSON objects with Unix timestamps.

### JSON Structure

```json
{
  "mode": "SINGLE",
  "timestamp": 1728930680,
  "temperature": 23.45,
  "humidity": 65.2
}
```

### Field Definitions

| Field       | Data Type | Range              | Description                    |
| ----------- | --------- | ------------------ | ------------------------------ |
| mode        | String    | SINGLE or PERIODIC | Measurement mode identifier    |
| timestamp   | Integer   | Unix epoch seconds | UTC time from DS3231 RTC       |
| temperature | Float     | -40.0 to 125.0     | Temperature in degrees Celsius |
| humidity    | Float     | 0.0 to 100.0       | Relative humidity percentage   |

### Output Timing

| Mode               | Output Rate                      | Triggering Mechanism                   |
| ------------------ | -------------------------------- | -------------------------------------- |
| Single measurement | Once per command                 | Immediate after measurement completion |
| Periodic mode      | Configurable (default 5 seconds) | Automatic from timer interrupt         |

Data transmission destination depends on ESP32 connection status:

- MQTT Connected: JSON sent directly to ESP32 via UART
- MQTT Disconnected: JSON buffered to SD card for later transmission

### Data Buffering and Synchronization

When ESP32 is offline, STM32 automatically buffers sensor data to SD card:

1. Measurement taken and timestamped
2. Data written to SD card circular buffer (204,800 records capacity)
3. Display updated with local status

When ESP32 reconnects and sends MQTT CONNECTED notification:

1. STM32 begins transmitting buffered data from SD card
2. Each buffered record sent as JSON via UART
3. After ESP32 acknowledges (by reading), record removed from buffer
4. Process continues until buffer is empty
5. New measurements resume normal transmission

SD card buffer structure:

- Record size: 512 bytes (one SD block)
- Capacity: 204,800 records
- Storage: Circular buffer with read/write indices
- Persistence: Metadata survives power cycles

### Status Messages

Command responses and system status:

```
UART OK                          # UART check response
SD Card Initialized              # SD card ready
SD Card Init Failed              # SD card error
Data buffered to SD              # Offline storage active
Buffered records: 1234           # SD card buffer status
```

## Display System

The ILI9225 TFT LCD (176x220 pixels) provides real-time system status visualization.

### Display Layout

```
+--------------------------------------------------+
|                     14:28:01                     |
|                   Th7 25/10/2025                 |
|                                                  |
|    28.5°C                     67% RH             |
|                                                  |
|    System       Periodic                         |
|    MQTT         Interval: 1s                     |
|                                                  |
+--------------------------------------------------+
```

### Display Updates

The display refreshes automatically when:

- New sensor measurement completes
- MQTT connection status changes
- Periodic mode starts or stops
- Measurement interval changes
- SD buffer count changes

Update frequency: Every 1-2 seconds during periodic mode, immediate on status changes.

### Display Elements

| Element     | Information                  | Update Trigger     |
| ----------- | ---------------------------- | ------------------ |
| Title bar   | System name and current time | Every second       |
| Temperature | Current reading in °C        | Each measurement   |
| Humidity    | Current reading in % RH      | Each measurement   |
| MQTT Status | Connected/Disconnected       | ESP32 notification |
| Mode        | IDLE/SINGLE/PERIODIC         | Mode change        |
| Interval    | Measurement period           | Interval change    |
| SD Buffer   | Number of stored records     | Data buffering     |

## System Operation

### Command Processing Flow

1. UART interrupt receives incoming bytes and stores them in the ring buffer
2. Main loop reads characters from ring buffer until newline character
3. Command tokenizer splits the command string into individual tokens
4. Command dispatcher searches command table for exact string match
5. Matched command handler validates parameters and calls sensor driver
6. Sensor driver performs I2C communication with SHT3X or DS3231
7. Data manager updates internal state and sets data ready flag
8. Next main loop iteration outputs JSON formatted data via UART

### Operating Modes

#### Single-Shot Mode

In single-shot mode, the system remains idle until a command is received. Each command triggers one measurement cycle and immediate output.

State transition:

```
IDLE → Receive Command → Measure → Update DataManager → Output JSON → IDLE
```

Timing: Command processing completes within 100 ms from reception to output.

#### Periodic Mode

In periodic mode, the sensor continuously samples at the configured rate, and the main loop outputs data every 5 seconds.

State transition:

```
IDLE → Start Periodic → Continuous Sampling → Output Every 5s → Stop Command → IDLE
```

The sensor internal buffer stores measurements at the specified rate (0.5 to 10 Hz). The firmware fetches data every 5 seconds regardless of sampling rate.

#### Mode Transitions

| Current State | Command             | New State | Behavior                                       |
| ------------- | ------------------- | --------- | ---------------------------------------------- |
| IDLE          | SHT3X SINGLE        | IDLE      | Single measurement, immediate output           |
| IDLE          | SHT3X PERIODIC      | PERIODIC  | Start continuous sampling                      |
| PERIODIC      | SHT3X SINGLE        | PERIODIC  | Temporary single measurement, periodic resumes |
| PERIODIC      | SHT3X PERIODIC STOP | IDLE      | Stop continuous sampling                       |
| Any           | SHT3X HEATER        | Unchanged | Heater control independent of mode             |

### Communication Parameters

| Parameter        | Value                    | Configuration                      |
| ---------------- | ------------------------ | ---------------------------------- |
| UART Baud Rate   | 115200 bps               | 8 data bits, no parity, 1 stop bit |
| UART Mode        | Interrupt RX, Polling TX | No DMA or hardware flow control    |
| I2C Bus Speed    | 100 kHz                  | Standard mode                      |
| I2C Addressing   | 7-bit                    | SHT3X: 0x44, DS3231: 0x68          |
| I2C Timeout      | 100 ms                   | Per transaction                    |
| Ring Buffer Size | 256 bytes                | UART receive buffer                |
| JSON Buffer Size | 128 bytes                | Output formatting buffer           |

### Performance Characteristics

| Metric                 | Typical Value | Maximum Value |
| ---------------------- | ------------- | ------------- |
| Command Response Time  | 50-100 ms     | 150 ms        |
| I2C Single Measurement | 4-15 ms       | 20 ms         |
| I2C Fetch Data         | 3-5 ms        | 10 ms         |
| JSON Formatting        | < 1 ms        | 2 ms          |
| UART Transmission      | ~8 ms         | 12 ms         |
| Main Loop Cycle        | 100-200 ms    | 500 ms        |

### Memory Usage

| Component           | RAM Usage  | Flash Usage |
| ------------------- | ---------- | ----------- |
| Ring Buffer         | 256 bytes  | -           |
| Command Line Buffer | 128 bytes  | -           |
| Print Buffer        | 256 bytes  | -           |
| Command Table       | ~200 bytes | ~800 bytes  |
| SHT3X Driver        | 24 bytes   | ~2 KB       |
| DS3231 Driver       | 8 bytes    | ~1 KB       |
| Data Manager        | 32 bytes   | ~1.5 KB     |
| Total Core Firmware | < 1 KB     | < 6 KB      |

Note: HAL library and system code require additional ~20 KB flash memory.

## Sensor Specifications

### SHT3X Temperature and Humidity Sensor

| Parameter               | Specification                           |
| ----------------------- | --------------------------------------- |
| Temperature Range       | -40°C to +125°C                         |
| Humidity Range          | 0% to 100% RH non-condensing            |
| Temperature Accuracy    | ±0.2°C typical at 25°C                  |
| Humidity Accuracy       | ±2% RH typical at 25°C, 20-80% RH range |
| Resolution              | 0.01°C, 0.01% RH                        |
| Response Time           | 2-8 seconds for 63% step change         |
| Repeatability HIGH      | ±0.04°C, ±0.015% RH                     |
| Repeatability MEDIUM    | ±0.08°C, ±0.024% RH                     |
| Repeatability LOW       | ±0.15°C, ±0.045% RH                     |
| Integrated Heater Power | 5.5 mW typical                          |
| I2C Interface           | 100 kHz standard mode                   |
| Supply Voltage          | 2.4V to 5.5V                            |

### DS3231 Real-Time Clock

| Parameter             | Specification                                   |
| --------------------- | ----------------------------------------------- |
| Accuracy              | ±2 ppm from 0°C to +40°C                        |
| Timekeeping           | Seconds, minutes, hours, day, date, month, year |
| Operating Temperature | -40°C to +85°C                                  |
| I2C Interface         | 100 kHz standard mode                           |
| Supply Voltage        | 2.3V to 5.5V                                    |
| Battery Backup        | CR2032 coin cell                                |

## Troubleshooting

### Common Issues

| Problem                            | Symptoms                           | Solution                                                       |
| ---------------------------------- | ---------------------------------- | -------------------------------------------------------------- |
| No serial output                   | Terminal shows no data             | Verify UART TX pin connection (PA9), check baud rate 115200    |
| Garbled characters                 | Random unreadable text             | Confirm baud rate matches 115200, verify common ground         |
| Commands not recognized            | UNKNOWN COMMAND response           | Check exact command spelling, commands are case-sensitive      |
| I2C communication failure          | FAILED error messages              | Verify 4.7kΩ pull-up resistors on I2C lines, check connections |
| Incorrect timestamp                | Wrong date or time values          | Set DS3231 time using DS3231 SETTIME command                   |
| Zero or invalid sensor readings    | Temperature and humidity show 0.00 | Verify SHT3X power supply, check ADDR pin connected to GND     |
| Periodic mode continues after stop | Data keeps outputting              | Send complete command: SHT3X PERIODIC STOP                     |

### Diagnostic Procedures

#### UART Loopback Test

Short PA9 (TX) to PA10 (RX). Any characters typed should echo back immediately.

#### I2C Bus Scan

Add temporary code to scan for I2C devices:

```c
for (uint8_t addr = 0x00; addr < 0x7F; addr++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 100) == HAL_OK) {
        PRINT_CLI("Found device at address: 0x%02X\r\n", addr);
    }
}
```

Expected results: 0x44 (SHT3X) and 0x68 (DS3231)

#### Verify Broker Status

Check that the Docker container is running:

```bash
docker ps | grep mqtt-broker
```

### Error Recovery

| Error Condition  | Manual Recovery                   | Automatic Recovery                            |
| ---------------- | --------------------------------- | --------------------------------------------- |
| I2C bus lockup   | Power cycle the system            | I2C peripheral reset (not implemented)        |
| UART buffer full | Stop sending commands temporarily | Ring buffer automatically overwrites old data |
| System hang      | Reset button or power cycle       | Watchdog timer reset (if enabled)             |

## Additional Documentation

This directory contains several supplementary documentation files:

- **FLOW_DIAGRAM.md** - Detailed control flow diagrams showing command processing logic and state transitions
- **SEQUENCE_DIAGRAM.md** - Timing and interaction sequence diagrams between firmware components
- **UML_CLASS_DIAGRAM.md** - Class structure and module relationship diagrams

## Quick Reference

### Most Common Commands

```
SHT3X SINGLE HIGH                 # Single measurement, high precision
SHT3X PERIODIC 1 HIGH             # Start 1 Hz continuous monitoring
SHT3X PERIODIC STOP               # Stop continuous monitoring
DS3231 SETTIME 2024 10 15 12 0 0  # Set real-time clock
DS3231 GETTIME                    # Read current time
```

### Hardware Checklist

- STM32F103 powered with 3.3V
- SHT3X sensor connected: SCL to PB6, SDA to PB7, ADDR pin to GND
- DS3231 RTC connected to shared I2C bus
- 4.7kΩ pull-up resistors installed on SCL and SDA lines
- UART connected: TX to PA9, RX to PA10
- Serial terminal configured: 115200 baud, 8 data bits, no parity, 1 stop bit

### Communication Parameters

| Parameter          | Value                                    |
| ------------------ | ---------------------------------------- |
| UART Baud Rate     | 115200 bps                               |
| UART Configuration | 8N1 (8 data bits, no parity, 1 stop bit) |
| I2C Bus Speed      | 100 kHz                                  |
| I2C Addressing     | 7-bit (SHT3X: 0x44, DS3231: 0x68)        |
| Response Latency   | < 150 ms                                 |
| Periodic Output    | Every 5 seconds                          |

## Documentation References

- Command Execute Library Details: [Command Execute Library](docs/cmd_execute.md)
- Command Function Library Details: [Command Function Library](docs/cmd_func.md)
- Command Parser Library Details: [Command Parser Library](docs/cmd_parser.md)
- Data Manager Library Details: [Data Manager Library](docs/data_manager.md)
- Display Library Details: [Display Library](docs/display.md)
- DS3231 RTC Driver Details: [DS3231 Library](docs/ds3231.md)
- Fonts Resource Details: [Fonts Library](docs/fonts.md)
- ILI9225 Display Driver Details: [ILI9225 Library](docs/ili9225.md)
- Print CLI (Debug Console) Details: [Print CLI Library](docs/print_cli.md)
- Ring Buffer (FIFO) Details: [Ring Buffer Library](docs/ring_buffer.md)
- SD Card Manager (Buffer + File Handling) Details: [SD Card Manager Library](docs/sd_card_manager.md)
- SD Card Low-Level Driver Details: [SD Card Library](docs/sd_card.md)
- Sensor JSON Output (Formatting) Details: [Sensor JSON Output Library](docs/sensor_json_output.md)
- SHT3X Sensor Driver Details: [SHT3X Library](docs/sht3x.md)
- UART Communication Interface Details: [UART Library](docs/uart.md)
- Wi-Fi Manager (ESP32 Communication) Details: [Wi-Fi Manager Library](docs/wifi_manager.md)
- STM32 Datalogger Library Details: [STM32 Datalogger Library](STM32/Datalogger_Lib/README.md)

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
