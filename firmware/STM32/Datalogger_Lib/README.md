# Datalogger Library

This library contains all the core modules and drivers for the STM32F103C8T6-based DATALOGGER system. It provides sensor interfaces, data management, communication protocols, display control, and SD card storage functionality.

## Overview

The Datalogger Library is a modular software framework designed to manage environmental data acquisition and communication.

## Library Structure

```
Datalogger_Lib/
├── docs/
│   ├── sht3x.md                  # SHT3X sensor driver documentation
│   ├── ds3231.md                 # DS3231 RTC driver documentation
│   ├── sd_card.md                # SD card driver documentation
│   ├── sd_card_manager.md        # SD card manager documentation
│   ├── ring_buffer.md            # Ring buffer documentation
│   ├── uart.md                   # UART handler documentation
│   ├── cmd_parser.md             # Command parser documentation
│   ├── command_execute.md        # Command execution documentation
│   ├── cmd_func.md               # Command functions documentation
│   ├── data_manager.md           # Data manager documentation
│   ├── sensor_json_output.md     # JSON output documentation
│   ├── display.md                # Display controller documentation
│   ├── ili9225.md                # ILI9225 LCD driver documentation
│   ├── fonts.md                  # Font system documentation
│   ├── wifi_manager.md           # MQTT manager documentation
│   └── print_cli.md              # CLI printing documentation
├── inc/
│   ├── sht3x.h                   # SHT3X temperature/humidity sensor driver
│   ├── ds3231.h                  # DS3231 RTC driver
│   ├── sd_card.h                 # Low-level SD card driver
│   ├── sd_card_manager.h         # High-level SD card data buffering
│   ├── ring_buffer.h             # Circular FIFO buffer
│   ├── uart.h                    # UART communication with ESP32
│   ├── cmd_parser.h              # Command parsing from ESP32
│   ├── command_execute.h         # Command execution engine
│   ├── cmd_func.h                # Command function implementations
│   ├── data_manager.h            # Centralized sensor data management
│   ├── sensor_json_output.h      # JSON formatting for sensor data
│   ├── display.h                 # Display controller
│   ├── ili9225.h                 # ILI9225 LCD driver
│   ├── fonts.h                   # Font definitions
│   ├── wifi_manager.h            # MQTT connection state tracking
│   └── print_cli.h               # CLI printing utilities
├── src/
│   ├── sht3x.c                   # Implements I2C communication and data readout for SHT3X sensors
│   ├── ds3231.c                  # Implements timekeeping, alarms, and temperature read for DS3231 RTC
│   ├── sd_card.c                 # Handles low-level SPI communication with SD cards
│   ├── sd_card_manager.c         # Manages buffered SD card logging and record storage
│   ├── ring_buffer.c             # Implements circular FIFO buffer for UART and data streams
│   ├── uart.c                    # Handles interrupt-driven UART reception and transmission
│   ├── cmd_parser.c              # Parses commands received from ESP32 interface
│   ├── command_execute.c         # Executes parsed commands and dispatches appropriate handlers
│   ├── cmd_func.c                # Defines command handler functions linked to specific operations
│   ├── data_manager.c            # Manages sensor readings and system mode/state
│   ├── sensor_json_output.c      # Formats and transmits sensor data in JSON format
│   ├── display.c                 # Controls layout, updates, and visual output on TFT LCD
│   ├── ili9225.c                 # Provides low-level SPI driver for ILI9225 LCD controller
│   ├── fonts.c                   # Contains bitmap font data used for text rendering
│   ├── wifi_manager.c            # Tracks MQTT connection state reported by ESP32
│   └── print_cli.c               # Provides formatted UART output similar to printf()
└── README.md                     # Library overview and component index
```

## Module Overview

### Sensor Drivers

**SHT3X Temperature and Humidity Sensor**

- I2C-based digital sensor driver
- Single-shot and periodic measurement modes
- Measurement repeatability control (high, medium, low)
- Configurable measurement rates (0.5, 1, 2, 4, 10 measurements per second)
- Built-in heater control for condensation removal
- CRC checksum validation
- Temperature range: -40 to 125 degrees Celsius
- Humidity range: 0 to 100 percent RH

**DS3231 Real-Time Clock**

- I2C-based precision RTC driver
- Battery backup support for timekeeping
- Two programmable alarms with interrupt output
- Temperature-compensated crystal oscillator
- Aging offset compensation
- 32kHz and square wave output options
- Temperature sensor output
- Oscillator stop flag detection

### Data Management

**Data Manager**

- Centralized sensor data collection and validation
- Mode management (IDLE, SINGLE, PERIODIC)
- Timestamp integration from RTC
- Data ready flag for transmission coordination
- JSON output generation
- Extensible for additional sensors

**Sensor JSON Output**

- Standardized JSON formatting for sensor measurements
- Mode identification (SINGLE or PERIODIC)
- Unix timestamp inclusion
- Temperature and humidity fields
- Consistent output format for ESP32 communication

**SD Card Manager**

- High-level data buffering interface
- Circular buffer implementation (204,800 records capacity)
- Persistent storage across power cycles
- Automatic overflow handling
- Read/write index management
- Sequence numbering for data integrity
- Record size: 512 bytes per entry (one SD block)

**SD Card Low-Level Driver**

- SPI-based SD card communication
- Block read and write operations
- Card initialization and detection
- CMD and ACMD command support
- Error handling and timeout management

### Communication

**UART Handler**

- Interrupt-driven UART reception from ESP32
- Ring buffer integration for data storage
- Command line parsing (newline-terminated)
- 115200 baud rate communication
- Receive complete callback implementation

**Ring Buffer**

- Circular FIFO buffer implementation
- Fixed size: 256 bytes
- ISR-safe operations with volatile indices
- Put, get, available, and clear functions
- Overflow prevention
- Zero-copy peek operations

**Command Parser**

- Text-based command interpreter
- Supported commands:
  - SINGLE: Trigger single measurement
  - PERIODIC ON: Start periodic measurements
  - PERIODIC OFF: Stop periodic measurements
  - SET TIME: Configure RTC date/time
  - SET PERIODIC INTERVAL: Adjust measurement interval
  - SHT3X HEATER: Control sensor heater
  - SHT3X ART: Run sensor self-test
  - SD CLEAR: Erase all buffered data
  - MQTT CONNECTED: Notification from ESP32
  - MQTT DISCONNECTED: Notification from ESP32
  - CHECK UART: UART status verification

**Command Execute**

- Command lookup table and dispatcher
- Argument parsing (argc/argv style)
- Function pointer invocation
- Error handling for unknown commands

**Command Functions**

- Implementation of all command handlers
- Sensor control functions
- RTC configuration functions
- Data transmission coordination
- System state management

### Display System

**Display Controller**

- Real-time status display management
- Dynamic data updates (temperature, humidity, time, status)
- MQTT connection status indicator
- Periodic mode indicator
- Measurement interval display
- Layout management and rendering

**ILI9225 LCD Driver**

- 176x220 pixel TFT LCD controller
- SPI communication interface
- 16-bit color support (RGB565)
- Hardware accelerated primitives:
  - Line drawing
  - Rectangle drawing (filled and outlined)
  - Circle drawing
  - Text rendering
- Display orientation control
- Backlight control
- Color inversion support

**Font System**

- Multiple font sizes support
- Bitmap font rendering
- Character spacing control
- Efficient storage format

### Utilities

**WiFi Manager**

- ESP32 WiFi status tracking
- Connection state monitoring
- Event-based status updates from ESP32

**Print CLI**

- Formatted output utilities
- Debug message printing
- UART transmission helpers

## Hardware Dependencies

The library is designed for:

- **MCU**: STM32F103C8T6 (ARM Cortex-M3, 72 MHz)
- **Flash**: 64 KB
- **RAM**: 20 KB
- **Peripherals Used**:
  - I2C1: SHT3X sensor and DS3231 RTC
  - SPI1: SD card and ILI9225 display
  - UART1: Communication with ESP32
  - TIM2: Periodic measurement timing

## Peripheral Configuration

**I2C1 Configuration**

- Speed: 100 kHz (standard mode)
- Pins: PB6 (SCL), PB7 (SDA)
- Pull-up resistors: 4.7k ohm external
- Devices:
  - SHT3X sensor: 0x44 (ADDR pin grounded)
  - DS3231 RTC: 0x68

**SPI1 Configuration**

- Speed: Up to 18 MHz (for SD card), 36 MHz (for display)
- Mode: Mode 0 (CPOL=0, CPHA=0)
- Pins: PA5 (SCK), PA6 (MISO), PA7 (MOSI)
- CS Pins:
  - SD Card: PA4
  - ILI9225: PB0

**UART1 Configuration**

- Baud Rate: 115200
- Data Bits: 8
- Stop Bits: 1
- Parity: None
- Flow Control: None
- Pins: PA9 (TX), PA10 (RX)

## Memory Usage

Approximate memory footprint:

**Flash (Code)**

- Sensor Drivers: ~4 KB
- SD Card Subsystem: ~6 KB
- Display Subsystem: ~8 KB
- Communication: ~5 KB
- Utilities: ~2 KB
- Total: ~25 KB

**RAM (Data)**

- Ring Buffer: 256 bytes
- SD Buffer Metadata: 16 bytes
- Display Frame Buffer: None (direct rendering)
- Sensor Structures: ~100 bytes
- Stack Usage: ~2 KB per task
- Total: ~3-4 KB

## Initialization Sequence

Proper initialization order is critical:

```c
void System_Init(void) {
    // 1. Initialize HAL and system clock
    HAL_Init();
    SystemClock_Config();

    // 2. Initialize peripherals
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_SPI1_Init();
    MX_UART1_Init();
    MX_TIM2_Init();

    // 3. Initialize library modules
    RingBuffer_Init(&uart_rx_buffer);           // UART buffer
    UART_Init(&huart1);                         // UART communication

    DS3231_Init(&g_ds3231, &hi2c1);             // RTC
    SHT3X_Init(&g_sht3x, &hi2c1, SHT3X_I2C_ADDR_GND); // Sensor

    SDCardManager_Init();                       // SD card system

    ILI9225_Init();                             // Display hardware
    display_init();                             // Display system

    DataManager_Init();                         // Data management

    // 4. Start periodic timer
    HAL_TIM_Base_Start_IT(&htim2);
}
```

## Usage Example

### Single Measurement

```c
void perform_single_measurement(void) {
    float temp, humidity;

    // Perform single-shot measurement
    sht3x_repeat_t repeat = SHT3X_HIGH;
    if (SHT3X_Single(&g_sht3x, &repeat, &temp, &humidity) == SHT3X_OK) {
        // Get current timestamp
        struct tm time;
        DS3231_Get_Time(&g_ds3231, &time);
        uint32_t timestamp = mktime(&time);

        // Update data manager
        DataManager_UpdateSingle(temp, humidity);

        // Print JSON to ESP32
        DataManager_Print();

        // Store to SD card if ESP32 offline
        if (!wifi_is_connected()) {
            SDCardManager_WriteData(timestamp, temp, humidity, "SINGLE");
        }

        // Update display
        display_update(timestamp, temp, humidity,
                      wifi_is_connected(), false, 0);
    }
}
```

### Periodic Measurement

```c
void start_periodic_mode(uint32_t interval_ms) {
    // Start periodic measurements
    sht3x_mode_t mode = SHT3X_PERIODIC_1MPS;
    sht3x_repeat_t repeat = SHT3X_HIGH;

    float temp, humidity;
    if (SHT3X_Periodic(&g_sht3x, &mode, &repeat, &temp, &humidity) == SHT3X_OK) {
        printf("Periodic mode started\n");
        periodic_mode_active = true;
        periodic_interval_ms = interval_ms;
    }
}

void periodic_measurement_task(void) {
    if (periodic_mode_active) {
        float temp, humidity;

        // Fetch data from sensor
        SHT3X_FetchData(&g_sht3x, &temp, &humidity);

        // Get timestamp
        struct tm time;
        DS3231_Get_Time(&g_ds3231, &time);
        uint32_t timestamp = mktime(&time);

        // Update data manager
        DataManager_UpdatePeriodic(temp, humidity);

        // Transmit to ESP32
        DataManager_Print();

        // Buffer to SD if ESP32 offline
        if (!wifi_is_connected()) {
            SDCardManager_WriteData(timestamp, temp, humidity, "PERIODIC");
        }

        // Update display
        display_update(timestamp, temp, humidity,
                      wifi_is_connected(), true,
                      periodic_interval_ms / 1000);
    }
}
```

### Command Processing

```c
void main_loop(void) {
    while (1) {
        // Handle incoming UART commands
        UART_Handle();

        // Process periodic measurements
        if (periodic_timer_expired) {
            periodic_measurement_task();
            periodic_timer_expired = false;
        }

        // Check for buffered data to send
        if (wifi_is_connected() && SDCardManager_GetBufferedCount() > 0) {
            sd_data_record_t record;
            if (SDCardManager_ReadData(&record)) {
                // Send buffered data to ESP32
                printf("{\"mode\":\"%s\",\"timestamp\":%lu,"
                      "\"temperature\":%.2f,\"humidity\":%.2f}\r\n",
                      record.mode, record.timestamp,
                      record.temperature, record.humidity);

                // Wait for acknowledgment, then remove
                SDCardManager_RemoveRecord();
            }
        }
    }
}
```

## Command Interface

Commands are received from ESP32 via UART and parsed by the command system:

**Measurement Commands**

```
SINGLE\r\n                        # Trigger single measurement
PERIODIC ON\r\n                   # Start periodic measurements
PERIODIC OFF\r\n                  # Stop periodic measurements
SET PERIODIC INTERVAL 5000\r\n    # Set interval to 5 seconds
```

**Configuration Commands**

```
SET TIME 2025 10 23 14 30 0\r\n   # Set RTC: YYYY MM DD HH MM SS
SET TIME 1760000000\r\n           # Set RTC: Timestamp
SHT3X HEATER ENABLE\r\n           # Enable sensor heater
SHT3X HEATER DISABLE\r\n          # Disable sensor heater
SHT3X ART\r\n                     # Run sensor self-test
```

**System Commands**

```
SD CLEAR\r\n                      # Clear all buffered data
CHECK UART\r\n                    # UART status check
MQTT CONNECTED\r\n                # Notification from ESP32
MQTT DISCONNECTED\r\n             # Notification from ESP32
```

## Data Flow Architecture

```
┌─────────────────────────────────┐
│   Sensors (SHT3X, DS3231)       │
└───────────────┬─────────────────┘
                │
                ↓
┌─────────────────────────────────┐
│          Data Manager           │
└──────┬─────────────────────┬────┘
       │                     │
       │                     │
       ↓                     ↓
┌──────────────┐  ┌─────────────────────┐
│ JSON Output  │  │  SD Card Manager    │
└──────┬───────┘  │  (if ESP32 offline) │
       │          └──────────┬──────────┘
       ↓                     │
┌──────────────┐             ↓
│     UART     │    ┌─────────────────┐
└──────┬───────┘    │ SD Card Storage │
       │            │    (buffer)     │
       ↓            └────────┬────────┘
┌──────────────┐             │
│     ESP32    │             │
│(if connected)│←────────────┘
└──────────────┘   Retrieve when
                   ESP32 reconnects
```

## Error Handling

All library functions return status codes for error detection:

**I2C Operations**

- Timeout detection
- NACK handling
- Bus error recovery

**SD Card Operations**

- Card initialization failure handling
- Write error detection
- Buffer overflow protection

**Sensor Operations**

- CRC validation
- Out-of-range detection
- Communication failure handling

Example error handling:

```c
if (SHT3X_Single(&g_sht3x, &repeat, &temp, &humidity) != SHT3X_OK) {
    printf("ERROR: Sensor read failed\n");

    // Send error indication (0.00 values)
    DataManager_UpdateSingle(0.0f, 0.0f);
    DataManager_Print();
}

if (!SDCardManager_WriteData(timestamp, temp, humidity, "SINGLE")) {
    printf("WARNING: SD card write failed - data lost\n");
}
```

## Performance Characteristics

**Measurement Times**

- Single-shot measurement: 15-30 ms (repeatability dependent)
- Periodic measurement fetch: 1-5 ms
- RTC read: 1-2 ms
- SD card write: 5-10 ms per block
- Display update: 50-100 ms (full screen)

**Power Consumption**

- Active mode: 30-40 mA @ 3.3V
- Sleep mode: Not implemented (planned for future)
- Sensor active: +1.5 mA
- Display active: +15-20 mA

**Communication Speeds**

- I2C: 100 kHz (standard mode)
- SPI: 18 MHz (SD card), 36 MHz (display)
- UART: 115200 baud

## Debugging

Enable debug output by defining DEBUG macro:

```c
#define DEBUG 1

#ifdef DEBUG
    #define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...)
#endif
```

Monitor UART output:

```bash
# Linux/Mac
screen /dev/ttyUSB0 115200

# Windows
putty -serial COM3 -sercfg 115200,8,n,1,N
```

## Testing

Individual module testing:

**Sensor Test**

```c
void test_sensor(void) {
    float temp, humidity;
    sht3x_repeat_t repeat = SHT3X_HIGH;

    for (int i = 0; i < 10; i++) {
        if (SHT3X_Single(&g_sht3x, &repeat, &temp, &humidity) == SHT3X_OK) {
            printf("Test %d: T=%.2f°C, H=%.2f%%\n", i, temp, humidity);
        }
        HAL_Delay(1000);
    }
}
```

**SD Card Test**

```c
void test_sd_card(void) {
    if (SDCardManager_Init()) {
        printf("SD card initialized\n");

        // Write test data
        for (int i = 0; i < 100; i++) {
            SDCardManager_WriteData(time(NULL), 25.0f + i * 0.1f,
                                   60.0f + i * 0.1f, "TEST");
        }

        printf("Written 100 records, buffered: %lu\n",
               SDCardManager_GetBufferedCount());
    }
}
```

## Future Enhancements

Planned improvements:

- Low power modes (sleep, stop)
- DMA for SPI transfers
- Additional sensor support (pressure, CO2)
- File system support on SD card (FAT32)
- Watchdog timer integration
- Bootloader for firmware updates

## Dependencies

- STM32 HAL Library (STM32F1xx)
- ARM CMSIS Core
- C Standard Library (newlib-nano)
- FatFs (optional, for file system support)

## Related Documentation

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

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
