# Print CLI Library

Provides formatted UART output utilities for CLI interaction and debugging.

## Overview

The Print CLI Library provides formatted text output over UART, similar to the standard `printf` function. It enables easy debugging and command-line interface communication with the Datalogger system.

## Files

- **print_cli.c**: Print CLI implementation
- **print_cli.h**: Print CLI API declaration

## Core Functionality

### Main Print Function

```c
void PRINT_CLI(char *fmt, ...);
```

Formatted print function over UART with printf-style syntax.

**Parameters**:

- `fmt`: Format string (same as printf)
- `...`: Variable arguments matching format specifiers

**Behavior**: Constructs formatted string in buffer and transmits via UART.

## Format Specifiers

Supports standard printf format specifiers:

### Integer Formats

```c
PRINT_CLI("Decimal: %d\r\n", 123);           // Output: "Decimal: 123"
PRINT_CLI("Unsigned: %u\r\n", 456);          // Output: "Unsigned: 456"
PRINT_CLI("Hex: 0x%X\r\n", 255);            // Output: "Hex: 0xFF"
PRINT_CLI("Hex: 0x%x\r\n", 255);            // Output: "Hex: 0xff"
PRINT_CLI("Octal: %o\r\n", 64);             // Output: "Octal: 100"
```

**Supported**:

- `%d`, `%i`: Signed decimal integer
- `%u`: Unsigned decimal integer
- `%x`: Hexadecimal (lowercase)
- `%X`: Hexadecimal (uppercase)
- `%o`: Octal

### Floating Point Formats

```c
PRINT_CLI("Float: %.2f\r\n", 25.456);        // Output: "Float: 25.46"
PRINT_CLI("Temp: %.1f C\r\n", 23.8);         // Output: "Temp: 23.8 C"
PRINT_CLI("Percent: %.0f%%\r\n", 65.2);      // Output: "Percent: 65%"
```

**Precision Control**:

- `%.0f`: No decimal places
- `%.1f`: 1 decimal place
- `%.2f`: 2 decimal places
- `%.3f`: 3 decimal places

### String Formats

```c
PRINT_CLI("Message: %s\r\n", "Hello");       // Output: "Message: Hello"
PRINT_CLI("Name: %10s\r\n", "STM32");        // Output: "Name:      STM32" (right-aligned)
PRINT_CLI("Name: %-10s\r\n", "STM32");       // Output: "Name: STM32     " (left-aligned)
```

**Supported**:

- `%s`: Null-terminated string
- `%10s`: Right-aligned in 10-character field
- `%-10s`: Left-aligned in 10-character field

### Character Format

```c
PRINT_CLI("Char: %c\r\n", 'A');              // Output: "Char: A"
```

### Pointer Format

```c
uint32_t *ptr = (uint32_t *)0x20000000;
PRINT_CLI("Address: %p\r\n", ptr);           // Output: "Address: 0x20000000"
```

## Buffer Configuration

### Buffer Size

```c
#define BUFFER_PRINT 128
```

**Important**: Maximum formatted string length is 128 bytes (including null terminator).

**Recommendation**: Keep messages under 120 characters to avoid truncation.

### Buffer Overflow Handling

If formatted string exceeds buffer size:

- String is truncated at 127 characters
- Null terminator added at position 127
- Transmission proceeds with truncated string

**Example**:

```c
// String > 128 bytes will be truncated
PRINT_CLI("This is a very long message that exceeds 128 bytes and will be truncated...");
```

## UART Configuration

### UART Handle

```c
extern UART_HandleTypeDef huart1;
```

**Default UART**: UART1 (configurable in print_cli.c)

### UART Parameters

Typical configuration (set in STM32CubeMX):

- **Baud Rate**: 115200
- **Word Length**: 8 bits
- **Stop Bits**: 1
- **Parity**: None
- **Flow Control**: None

### Transmission Mode

Uses **blocking transmission**:

```c
HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
```

**Characteristics**:

- Waits until entire string is transmitted
- Uses HAL_MAX_DELAY (infinite timeout)
- Safe for use in main loop and non-ISR contexts

## Usage Examples

### Basic Output

```c
// Simple message
PRINT_CLI("System initialized\r\n");

// Variable output
uint8_t status = 1;
PRINT_CLI("Status: %d\r\n", status);
```

### Sensor Data Output

```c
float temperature = 25.5;
float humidity = 65.2;

PRINT_CLI("Sensor Data:\r\n");
PRINT_CLI("  Temperature: %.2f C\r\n", temperature);
PRINT_CLI("  Humidity: %.2f %%\r\n", humidity);
```

**Output**:

```
Sensor Data:
  Temperature: 25.50 C
  Humidity: 65.20 %
```

### Formatted Table

```c
PRINT_CLI("Time       Temp    Hum\r\n");
PRINT_CLI("--------------------------------\r\n");
PRINT_CLI("14:35:22   %.1f   %.1f\r\n", temp1, hum1);
PRINT_CLI("14:35:27   %.1f   %.1f\r\n", temp2, hum2);
PRINT_CLI("14:35:32   %.1f   %.1f\r\n", temp3, hum3);
```

**Output**:

```
Time       Temp    Hum
--------------------------------
14:35:22   25.5   65.2
14:35:27   25.6   65.1
14:35:32   25.7   65.0
```

### Error Messages

```c
uint8_t errorCode = 3;
PRINT_CLI("[ERROR] Sensor read failed (code: %d)\r\n", errorCode);

PRINT_CLI("[WARNING] SD card near full: %u%% used\r\n", usagePercent);

PRINT_CLI("[INFO] Periodic mode started, interval: %d seconds\r\n", interval);
```

### Debug Output

```c
void DebugPrintRegisters(void)
{
    PRINT_CLI("Register Dump:\r\n");
    PRINT_CLI("  R0: 0x%08X\r\n", register0);
    PRINT_CLI("  R1: 0x%08X\r\n", register1);
    PRINT_CLI("  R2: 0x%08X\r\n", register2);
}
```

### Command Responses

```c
void SINGLE_PARSER(uint8_t argc, char **argv)
{
    float temp, hum;

    if (SHT3X_Read_TempHumi(&temp, &hum) == 0)
    {
        PRINT_CLI("OK: Temp=%.2f C, Hum=%.2f %%\r\n", temp, hum);
    }
    else
    {
        PRINT_CLI("ERROR: Sensor read failed\r\n");
    }
}
```

## Line Ending Conventions

### Carriage Return + Line Feed

Use `\r\n` for proper terminal display:

```c
PRINT_CLI("Line 1\r\n");   // Correct
PRINT_CLI("Line 2\r\n");

// Output in terminal:
// Line 1
// Line 2
```

**Avoid**:

```c
PRINT_CLI("Line 1\n");     // May not display correctly in some terminals
```

**Why**: Most terminal emulators expect CRLF (`\r\n`) for proper cursor positioning.

## Performance Characteristics

### Execution Time

| String Length | Transmission Time @ 115200 |
| ------------- | -------------------------- |
| 10 bytes      | ~0.9 ms                    |
| 50 bytes      | ~4.3 ms                    |
| 128 bytes     | ~11 ms                     |

**Formula**: Time (ms) = (bytes × 10 bits/byte) / (115200 bits/sec) × 1000

### CPU Blocking

**Important**: `PRINT_CLI` blocks CPU during transmission.

**Example**:

```c
PRINT_CLI("Long message...\r\n");  // CPU blocked for ~5ms
// Code here executes after transmission complete
```

**Recommendation**: Avoid excessive printing in time-critical code.

## Thread Safety

### Not Thread-Safe

The library uses a **static buffer**:

```c
static char buffer[BUFFER_PRINT];
```

**Problem**: Concurrent calls from ISR and main loop can corrupt buffer.

**Safe Usage**:

```c
// Main loop
while (1)
{
    PRINT_CLI("Main loop message\r\n");  // OK
}
```

**Unsafe Usage**:

```c
// ISR context
void EXTI_IRQHandler(void)
{
    PRINT_CLI("Interrupt occurred\r\n");  // UNSAFE - may corrupt buffer
}
```

**Solution**: Only call `PRINT_CLI` from main loop or single thread.

## Comparison with printf

### Similarities

- Same format specifier syntax
- Variable argument support
- Type conversion

### Differences

| Feature         | printf           | PRINT_CLI         |
| --------------- | ---------------- | ----------------- |
| Output          | stdout (console) | UART              |
| Buffer size     | Typically 1024+  | 128 bytes         |
| Floating point  | Full support     | Limited precision |
| Return value    | Character count  | void              |
| Newlib required | Yes              | No                |

**Advantage of PRINT_CLI**: No newlib dependency, smaller code size.

## Integration with Command Parser

### Command Response Pattern

```c
void COMMAND_PARSER(uint8_t argc, char **argv)
{
    if (argc != expected)
    {
        PRINT_CLI("Usage: COMMAND <arg1> <arg2>\r\n");
        return;
    }

    // Execute command
    uint8_t result = DoSomething();

    if (result == 0)
    {
        PRINT_CLI("Success\r\n");
    }
    else
    {
        PRINT_CLI("Error: %d\r\n", result);
    }
}
```

### Status Reporting

```c
void ReportSystemStatus(void)
{
    PRINT_CLI("\r\n--- System Status ---\r\n");
    PRINT_CLI("Mode: %s\r\n", mode == PERIODIC_MODE ? "PERIODIC" : "SINGLE");
    PRINT_CLI("Temperature: %.2f C\r\n", DATA_MANAGER_Get_Temperature());
    PRINT_CLI("Humidity: %.2f %%\r\n", DATA_MANAGER_Get_Humidity());
    PRINT_CLI("Free SD space: %u MB\r\n", SD_GetFreeSpace());
    PRINT_CLI("-------------------\r\n");
}
```

## Error Handling

### UART Transmission Errors

The library does not check HAL_UART_Transmit return value:

```c
HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), HAL_MAX_DELAY);
// No error checking
```

**Improved Version** (if needed):

```c
void PRINT_CLI_Safe(char *fmt, ...)
{
    char buffer[BUFFER_PRINT];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, BUFFER_PRINT, fmt, args);
    va_end(args);

    // Check transmission result
    if (HAL_UART_Transmit(&huart1, (uint8_t *)buffer, strlen(buffer), 100) != HAL_OK)
    {
        // Handle error (e.g., retry or log)
    }
}
```

## Memory Footprint

### Flash Usage

- Function code: ~500 bytes
- vsnprintf implementation: ~3-4 KB (from C library)

**Total**: ~4-5 KB

### RAM Usage

- Static buffer: 128 bytes
- Stack usage: ~50 bytes (during function call)

**Total**: ~180 bytes

## Best Practices

### 1. Keep Messages Concise

```c
// Good: 45 bytes
PRINT_CLI("Temp: %.1f C, Hum: %.1f %%\r\n", temp, hum);

// Avoid: Excessive verbosity
PRINT_CLI("The current temperature reading from the SHT3X sensor is: %.1f degrees Celsius\r\n", temp);
```

### 2. Use Appropriate Precision

```c
// Good: 1 decimal place sufficient for temperature
PRINT_CLI("Temp: %.1f C\r\n", 25.456);  // "Temp: 25.5 C"

// Overkill: Excessive precision
PRINT_CLI("Temp: %.6f C\r\n", 25.456);  // "Temp: 25.456000 C"
```

### 3. Batch Related Output

```c
// Good: Single function call
PRINT_CLI("Sensor: Temp=%.1f C, Hum=%.1f %%\r\n", temp, hum);

// Less efficient: Multiple calls
PRINT_CLI("Temp: %.1f C\r\n", temp);
PRINT_CLI("Hum: %.1f %%\r\n", hum);
```

### 4. Add Context to Debug Messages

```c
// Good: Clear context
PRINT_CLI("[SD_CARD] Write failed: sector %u\r\n", sector);

// Poor: Unclear origin
PRINT_CLI("Write failed\r\n");
```

### 5. Use Conditional Compilation for Debug

```c
#ifdef DEBUG
    #define DEBUG_PRINT(...) PRINT_CLI(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)  // No-op in release build
#endif

// Usage
DEBUG_PRINT("Debug: Variable x = %d\r\n", x);  // Only in debug builds
```

## Alternative Implementations

### Non-Blocking UART (DMA)

For high-throughput applications:

```c
void PRINT_CLI_DMA(char *fmt, ...)
{
    static char buffer[BUFFER_PRINT];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, BUFFER_PRINT, fmt, args);
    va_end(args);

    HAL_UART_Transmit_DMA(&huart1, (uint8_t *)buffer, strlen(buffer));
    // Non-blocking, continues immediately
}
```

**Benefit**: CPU not blocked during transmission.

**Caution**: Requires DMA configuration and buffer management.

## Dependencies

### Required

- **STM32 HAL UART**: UART transmission functions
- **C standard library**: vsnprintf for formatting

### Used By

- **cmd_parser.c**: All command parsers use PRINT_CLI for responses
- **main.c**: Status and error messages
- **sensor_json_output.c**: JSON output transmission

## Summary

The Print CLI Library provides:

- Printf-style formatted output over UART
- Support for all standard format specifiers
- 128-byte buffer for message construction
- Simple API with single function call
- No newlib dependency (lightweight)
- Easy debugging and CLI communication

This library simplifies UART communication and enables human-readable output for debugging, status reporting, and command-line interfaces in the Datalogger system.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
