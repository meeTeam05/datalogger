# STM32 UART Component

This component provides UART communication between the ESP32 and STM32 microcontroller in the DATALOGGER system. It handles bidirectional data exchange with the STM32 sensor node, including command transmission and JSON sensor data reception.

## Component Files

```
stm32_uart/
├── stm32_uart.h           # Public API header with structure and function declarations
├── stm32_uart.c           # Implementation of UART communication and data processing
├── CMakeLists.txt         # ESP-IDF build configuration
├── component.mk           # Legacy build system support
├── Kconfig                # Configuration menu options
└── README.md              # This file
```

## Overview

The STM32 UART component establishes asynchronous serial communication between ESP32 and STM32 at 115200 baud. It receives JSON-formatted sensor data from STM32 and transmits sensor control commands from ESP32. A dedicated FreeRTOS task continuously processes incoming data using a ring buffer for ISR-safe reception.

## Key Features

- Configurable UART port, baud rate, and GPIO pins via Kconfig
- Ring buffer implementation for interrupt-safe data reception
- Line-based message processing with newline detection
- Background FreeRTOS task for continuous data monitoring
- Callback mechanism for application-level data handling
- Command transmission with proper line termination
- Hardware initialization with error checking

## Hardware Connection

The ESP32 and STM32 communicate via UART with the following default connections:

```
ESP32 GPIO17 (TX) -----> STM32 PA10 (RX)
ESP32 GPIO16 (RX) <----- STM32 PA9  (TX)
ESP32 GND        <-----> STM32 GND
```

UART parameters:

- Baud rate: 115200 bps (configurable via menuconfig)
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None

## Data Structure

```c
typedef struct {
    int uart_num;                        // UART port number (default: UART2)
    int baud_rate;                       // Baud rate (default: 115200)
    int tx_pin;                          // TX GPIO pin (default: 17)
    int rx_pin;                          // RX GPIO pin (default: 16)
    ring_buffer_t rx_buffer;             // Ring buffer for received data
    stm32_data_callback_t data_callback; // Callback function for complete lines
    bool initialized;                    // Initialization status flag
} stm32_uart_t;
```

## API Functions

### Initialization

**STM32_UART_Init**

```c
bool STM32_UART_Init(stm32_uart_t *uart,
                     int uart_num,
                     int baud_rate,
                     int tx_pin,
                     int rx_pin,
                     stm32_data_callback_t callback);
```

Initializes UART hardware and prepares the communication channel.

Parameters:

- uart: Pointer to STM32 UART structure
- uart_num: UART port number (typically UART_NUM_2)
- baud_rate: Communication speed in bits per second (115200)
- tx_pin: GPIO pin for transmission
- rx_pin: GPIO pin for reception
- callback: Function to call when complete line received

Returns:

- true: Initialization successful
- false: Initialization failed (check GPIO availability and UART driver)

This function configures the UART peripheral, allocates driver resources, installs UART interrupt handlers, and initializes the ring buffer for data reception.

### Command Transmission

**STM32_UART_SendCommand**

```c
bool STM32_UART_SendCommand(stm32_uart_t *uart, const char *command);
```

Sends a command string to the STM32 with automatic newline termination.

Parameters:

- uart: Pointer to initialized STM32 UART structure
- command: Null-terminated command string (e.g., "SHT3X SINGLE HIGH")

Returns:

- true: Command sent successfully
- false: Send failed (UART not initialized or transmission error)

The function automatically appends '\n' line terminator to the command. Maximum command length is defined by STM32_UART_MAX_LINE_LENGTH.

### Data Processing

**STM32_UART_ProcessData**

```c
void STM32_UART_ProcessData(stm32_uart_t *uart);
```

Processes received data from ring buffer and extracts complete lines.

Parameters:

- uart: Pointer to STM32 UART structure

This function reads bytes from the ring buffer, accumulates them until a newline character is detected, then invokes the registered callback with the complete line. Called continuously by the background task.

**STM32_UART_StartTask**

```c
bool STM32_UART_StartTask(stm32_uart_t *uart);
```

Creates a dedicated FreeRTOS task for continuous data reception processing.

Parameters:

- uart: Pointer to STM32 UART structure

Returns:

- true: Task created successfully
- false: Task creation failed

The task runs at priority 10 with 4KB stack, continuously calling STM32_UART_ProcessData() to check for incoming data.

### Cleanup

**STM32_UART_Deinit**

```c
void STM32_UART_Deinit(stm32_uart_t *uart);
```

Deinitializes UART hardware and frees allocated resources.

Parameters:

- uart: Pointer to STM32 UART structure

Uninstalls UART driver and releases GPIO pins for other uses.

## Usage Example

```c
#include "stm32_uart.h"

// Declare UART structure
stm32_uart_t stm32_uart;

// Callback function for received lines
void on_stm32_data(const char *line) {
    printf("Received from STM32: %s\n", line);

    // Parse JSON sensor data
    // JSON_Parser_ProcessLine(&parser, line);
}

void app_main(void) {
    // Initialize UART communication
    bool init_ok = STM32_UART_Init(&stm32_uart,
                                   UART_NUM_2,    // UART port
                                   115200,        // Baud rate
                                   17,            // TX pin
                                   16,            // RX pin
                                   on_stm32_data  // Callback
                                  );

    if (!init_ok) {
        printf("UART initialization failed!\n");
        return;
    }

    // Start background processing task
    if (!STM32_UART_StartTask(&stm32_uart)) {
        printf("Failed to start UART task!\n");
        return;
    }

    // Send commands to STM32
    STM32_UART_SendCommand(&stm32_uart, "SHT3X SINGLE HIGH");

    vTaskDelay(pdMS_TO_TICKS(5000));

    STM32_UART_SendCommand(&stm32_uart, "SHT3X PERIODIC 1 HIGH");
}
```

## Configuration via Menuconfig

Configure component parameters in ESP-IDF menuconfig:

```
Component config → STM32 Communication (UART)
```

Available options:

- UART Port Number (default: 2)
- Baud Rate (default: 115200)
- TX GPIO Pin (default: 17)
- RX GPIO Pin (default: 16)

## Data Flow

### Reception Flow

1. STM32 transmits JSON data via UART TX pin
2. ESP32 UART hardware receives bytes and triggers interrupt
3. UART ISR stores bytes in ring buffer
4. Background task reads ring buffer
5. Task accumulates bytes until newline character
6. Complete line passed to registered callback function
7. Callback processes JSON data (typically via json_sensor_parser)

### Transmission Flow

1. Application calls STM32_UART_SendCommand()
2. Function appends newline to command string
3. UART driver transmits bytes to STM32
4. STM32 receives command via UART RX pin
5. STM32 firmware parses and executes command
6. STM32 responds with JSON sensor data

## Line Processing Details

The component implements line-based message processing:

- Accumulates received bytes in a temporary line buffer
- Detects line termination on '\r' (carriage return) or '\n' (newline)
- Null-terminates the line string before callback invocation
- Resets line buffer for next message
- Handles buffer overflow by discarding excess characters
- Ignores empty lines and whitespace-only lines

Maximum line length: 128 characters (STM32_UART_MAX_LINE_LENGTH)

## Integration with Other Components

This component integrates with:

**Ring Buffer Component**: Uses ring_buffer.h for ISR-safe data storage

**JSON Sensor Parser Component**: Callback typically passes lines to JSON_Parser_ProcessLine()

**MQTT Handler Component**: Parsed sensor data published to MQTT topics

## Error Handling

The component handles various error conditions:

**UART Initialization Failure**

- Check GPIO pin availability
- Verify UART port not already in use
- Confirm ESP-IDF UART driver compiled

**Command Send Failure**

- Verify UART initialization completed
- Check STM32 is powered and connected
- Monitor for transmission errors in esp_log output

**Missing Data**

- Verify TX/RX pins correctly cross-connected
- Check common ground connection
- Confirm baud rate matches STM32 (115200)
- Monitor ring buffer overflow (indicates data loss)

**Garbled Data**

- Verify baud rate configuration
- Check for electromagnetic interference
- Ensure proper signal integrity (short cables, twisted pairs)

## Performance Characteristics

- UART Baud Rate: 115200 bps (approximately 11.5 KB/s theoretical maximum)
- Ring Buffer Size: 256 bytes
- Maximum Line Length: 128 characters
- Task Processing Period: Continuous (no delay between reads)
- Memory Usage: Approximately 512 bytes per instance
- CPU Utilization: Less than 1% at typical data rates

## Limitations

- Single UART port supported per instance
- No hardware flow control
- Fixed line termination characters ('\r' or '\n')
- Maximum line length cannot be exceeded
- Ring buffer overflow causes data loss without notification
- Not suitable for binary protocols (line-based text only)

## Debugging

Enable ESP-IDF logging for detailed information:

```c
// In sdkconfig or via menuconfig
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y

// Component-specific logging
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
```

Monitor UART activity:

```bash
idf.py monitor | grep "STM32_UART"
```

Check for ring buffer overflows, transmission errors, and received line contents.

## Dependencies

- ESP-IDF UART driver (driver/uart.h)
- Ring Buffer Component (ring_buffer.h)
- FreeRTOS (freertos/FreeRTOS.h, freertos/task.h)
- ESP-IDF logging (esp_log.h)

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
