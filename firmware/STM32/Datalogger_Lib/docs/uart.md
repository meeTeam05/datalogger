# UART Handler Library

Handles interrupt-driven UART communication and command buffering for incoming data streams.

## Overview

The UART Handler Library provides interrupt-driven UART reception and command buffering for the Datalogger system. It receives data from the ESP32 module via UART, buffers complete commands, and triggers command execution when a command line is fully received.

## Files

- **uart.c**: UART handler implementation
- **uart.h**: UART handler API declarations and global variables

## Core Functionality

### UART Configuration

**Default UART**: UART1
**Typical Settings**:

- Baud Rate: 115200
- Data Bits: 8
- Stop Bits: 1
- Parity: None
- Flow Control: None

### Pin Connections

| STM32 Pin | ESP32 Pin | Description             |
| --------- | --------- | ----------------------- |
| USART1_TX | RX        | UART Transmit → Receive |
| USART1_RX | TX        | UART Receive ← Transmit |
| GND       | GND       | Common Ground           |

## Global Variables

### UART Handle

```c
extern UART_HandleTypeDef huart1;
```

Main UART peripheral handle (configured in STM32CubeMX).

### Receive Buffer

```c
extern uint8_t buff[BUFFER_UART];  // Command buffer (128 bytes)
extern uint8_t index_uart;          // Current write position in buffer
```

**Buffer Size**:

```c
#define BUFFER_UART 128
```

**Capacity**: 128 bytes (sufficient for typical command strings)

### Single-Byte Reception

```c
extern uint8_t data_rx;  // Temporary storage for incoming byte
```

Used by interrupt handler to receive one byte at a time.

### Command Ready Flag

```c
extern uint8_t Flag_UART;  // 1 = complete command received, 0 = waiting
```

Signals main loop that a complete command is ready for processing.

## API Functions

### UART Initialization

```c
void UART_Init(UART_HandleTypeDef *huart);
```

Initializes UART interrupt reception.

**Parameters**:

- `huart`: Pointer to UART handle

**Operations**:

1. Clears receive buffer
2. Resets buffer index
3. Clears command ready flag
4. Starts interrupt-based reception

**Usage Example**:

```c
int main(void)
{
    HAL_Init();
    MX_USART1_UART_Init();

    UART_Init(&huart1);  // Start receiving

    while (1)
    {
        UART_Handle();  // Process received data
    }
}
```

### UART Reception Callback

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
```

HAL callback function, called automatically when a byte is received.

**Behavior**:

1. Triggered by UART receive interrupt
2. Stores received byte in buffer: `buff[index_uart++] = data_rx`
3. Re-enables interrupt for next byte
4. **Does NOT check for command terminator** (handled in `UART_Handle()`)

**Implementation**:

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        // Store received byte
        buff[index_uart++] = data_rx;

        // Prevent buffer overflow
        if (index_uart >= BUFFER_UART)
        {
            index_uart = 0;  // Wrap around (or could stop reception)
        }

        // Re-enable reception for next byte
        HAL_UART_Receive_IT(&huart1, &data_rx, 1);
    }
}
```

**ISR Context**: This function runs in interrupt context. Keep it short!

### UART Command Handler

```c
void UART_Handle(void);
```

Processes received data in main loop (not ISR).

**Behavior**:

1. Checks for command terminator (`\r` or `\n`)
2. If found:
   - Null-terminates command string
   - Sets `Flag_UART = 1`
3. Main loop can then execute command

**Usage Example**:

```c
void main(void)
{
    UART_Init(&huart1);

    while (1)
    {
        UART_Handle();  // Check for complete commands

        if (Flag_UART)
        {
            Flag_UART = 0;

            // Execute command
            COMMAND_EXECUTE((char *)buff);

            // Clear buffer
            memset(buff, 0, sizeof(buff));
            index_uart = 0;
        }
    }
}
```

## Command Reception Flow

### Step-by-Step Process

1. **Character Reception** (ISR):

   ```
   User types: S I N G L E \r

   ISR called 7 times:
     1. buff[0] = 'S', index_uart = 1
     2. buff[1] = 'I', index_uart = 2
     3. buff[2] = 'N', index_uart = 3
     4. buff[3] = 'G', index_uart = 4
     5. buff[4] = 'L', index_uart = 5
     6. buff[5] = 'E', index_uart = 6
     7. buff[6] = '\r', index_uart = 7
   ```

2. **Command Detection** (Main Loop):

   ```c
   UART_Handle() detects '\r' at buff[6]
   → Replaces '\r' with '\0'
   → Sets Flag_UART = 1
   ```

3. **Command Execution** (Main Loop):
   ```c
   if (Flag_UART)
   {
       COMMAND_EXECUTE("SINGLE");  // buff now contains "SINGLE\0"
       // Clear buffer for next command
   }
   ```

## Command Terminator Handling

### Accepted Terminators

- **Carriage Return**: `\r` (0x0D)
- **Line Feed**: `\n` (0x0A)
- **Both**: `\r\n` (Windows-style line ending)

**Detection Logic**:

```c
void UART_Handle(void)
{
    for (uint8_t i = 0; i < index_uart; i++)
    {
        if (buff[i] == '\r' || buff[i] == '\n')
        {
            buff[i] = '\0';  // Null-terminate
            Flag_UART = 1;
            return;
        }
    }
}
```

### Example Commands

```
"SINGLE\r"                → Command: "SINGLE"
"PERIODIC ON\n"           → Command: "PERIODIC ON"
"SET TIME 1760000000\r\n" → Command: "SET TIME 1760000000"
```

## Buffer Management

### Buffer Overflow Protection

**Current Behavior**: Buffer wraps at index 127:

```c
if (index_uart >= BUFFER_UART)
{
    index_uart = 0;  // Wrap to start (overwrites old data)
}
```

**Improved Protection**:

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        if (index_uart < BUFFER_UART - 1)  // Leave room for null terminator
        {
            buff[index_uart++] = data_rx;
        }
        else
        {
            // Buffer full, discard character or signal error
            overflow_count++;
        }

        HAL_UART_Receive_IT(&huart1, &data_rx, 1);
    }
}
```

### Buffer Clear After Command

**Important**: Clear buffer after command execution to prevent command replay.

```c
if (Flag_UART)
{
    Flag_UART = 0;

    COMMAND_EXECUTE((char *)buff);

    // Clear buffer
    memset(buff, 0, BUFFER_UART);
    index_uart = 0;
}
```

## Typical Command Examples

### Single Measurement

```
ESP32 sends:  "SINGLE\r\n"
STM32 receives: buff = "SINGLE\0\n"
Executed: SINGLE_PARSER()
Response: {"mode":"SINGLE","timestamp":1710000000,"temperature":25.50,"humidity":65.20}
```

### Set Time

```
ESP32 sends: "SET TIME 1760000000\r\n"
STM32 receives: buff = "SET TIME 1760000000\0"
Executed: SET_TIME_PARSER(argc=3, argv=["SET","TIME","1760000000"])
Response: "Time set successfully\r\n"
```

### Periodic Mode Control

```
ESP32 sends: "PERIODIC ON\r\n"
STM32 receives: buff = "PERIODIC ON\0"
Executed: PERIODIC_ON_PARSER()
Response: "Periodic mode enabled\r\n"

[After 5 seconds, timer triggers]
STM32 sends: {"mode":"PERIODIC""timestamp":1710000005,"temperature":25.52,"humidity":65.18}
```

## Integration with Command Execution

### Main Loop Pattern

```c
int main(void)
{
    // System initialization
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_I2C1_Init();

    // Initialize peripherals
    SHT3X_Init(&g_sht3x, &hi2c1, 0x44);
    DS3231_Init();
    SDCardManager_Init();
    DISPLAY_Init();

    // Start UART reception
    UART_Init(&huart1);

    // Main loop
    while (1)
    {
        // Handle UART commands
        UART_Handle();
        if (Flag_UART)
        {
            Flag_UART = 0;
            COMMAND_EXECUTE((char *)buff);
            memset(buff, 0, BUFFER_UART);
            index_uart = 0;
        }

        // Handle periodic measurements
        if (Flag_Periodic)
        {
            Flag_Periodic = 0;
            PeriodicMeasurement();
        }

        // Update display
        static uint32_t lastUpdate = 0;
        if (HAL_GetTick() - lastUpdate > 500)
        {
            lastUpdate = HAL_GetTick();
            DISPLAY_Update();
        }
    }
}
```

## Performance Characteristics

### Interrupt Overhead

**Per Byte Received**:

- ISR entry/exit: ~1 µs
- Store byte in buffer: ~0.5 µs
- Re-enable reception: ~1 µs
- **Total**: ~2.5 µs per byte

### UART Reception Time

**@ 115200 baud**:

- Bit time: 1/115200 ≈ 8.68 µs
- Byte time: 8.68 µs × 10 bits ≈ 87 µs (8 data + 1 start + 1 stop)

**Command Reception Example**:

```
Command: "SINGLE\r\n" (8 bytes)
Time = 8 × 87 µs = 696 µs ≈ 0.7 ms
```

### CPU Utilization

For typical command rate (1 command per second):

- Interrupt time per command: 8 bytes × 2.5 µs = 20 µs
- Utilization: 20 µs / 1,000,000 µs = 0.002%

**Negligible CPU overhead.**

## Error Handling

### Buffer Overflow

```c
// Detect overflow
if (index_uart >= BUFFER_UART)
{
    printf("WARNING: UART buffer overflow\n");

    // Clear buffer and restart
    memset(buff, 0, BUFFER_UART);
    index_uart = 0;
    Flag_UART = 0;
}
```

### Timeout Detection

Detect incomplete commands:

```c
static uint32_t lastRxTime = 0;

void UART_Handle(void)
{
    // Check for command terminator
    // ...

    // Check for timeout (no terminator received)
    if (index_uart > 0 && (HAL_GetTick() - lastRxTime) > 1000)
    {
        printf("Command timeout, clearing buffer\n");
        memset(buff, 0, BUFFER_UART);
        index_uart = 0;
    }

    if (index_uart > 0)
    {
        lastRxTime = HAL_GetTick();
    }
}
```

### Lost Data Detection

```c
static uint32_t rx_byte_count = 0;
static uint32_t rx_overflow_count = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        rx_byte_count++;

        if (index_uart < BUFFER_UART - 1)
        {
            buff[index_uart++] = data_rx;
        }
        else
        {
            rx_overflow_count++;
        }

        HAL_UART_Receive_IT(&huart1, &data_rx, 1);
    }
}

void PrintUARTStats(void)
{
    printf("RX bytes: %lu, Overflows: %lu\n", rx_byte_count, rx_overflow_count);
}
```

## Best Practices

### 1. Clear Buffer After Use

```c
// Always clear buffer after command execution
COMMAND_EXECUTE((char *)buff);
memset(buff, 0, BUFFER_UART);
index_uart = 0;
```

### 2. Check Flag Regularly

```c
// In main loop, check Flag_UART frequently
while (1)
{
    UART_Handle();  // Process UART data

    if (Flag_UART)
    {
        // Handle immediately
        ProcessCommand();
    }

    // Other tasks
}
```

### 3. Protect Against Long Commands

```c
// Limit command length
if (index_uart > 100)  // Assume max command is 100 chars
{
    printf("Command too long, discarding\n");
    memset(buff, 0, BUFFER_UART);
    index_uart = 0;
}
```

### 4. Validate Command Before Execution

```c
if (Flag_UART)
{
    Flag_UART = 0;

    // Validate null-termination
    buff[BUFFER_UART - 1] = '\0';  // Ensure null-terminated

    // Check command length
    if (strlen((char *)buff) > 0)
    {
        COMMAND_EXECUTE((char *)buff);
    }

    // Clear buffer
    memset(buff, 0, BUFFER_UART);
    index_uart = 0;
}
```

## Thread Safety

### ISR and Main Loop Interaction

**Safe Scenario**:

- **ISR**: Writes to `buff[]` and `index_uart`
- **Main Loop**: Reads from `buff[]` after `Flag_UART` is set

**Race Condition**: If main loop modifies buffer while ISR is writing.

**Solution**: Disable interrupts during critical operations:

```c
// Critical section: clear buffer
__disable_irq();
memset(buff, 0, BUFFER_UART);
index_uart = 0;
__enable_irq();
```

## Memory Footprint

### RAM Usage

- Command buffer: 128 bytes
- Buffer index: 1 byte
- Flag: 1 byte
- Single-byte receive: 1 byte
- **Total**: 131 bytes

### Flash Usage

- Function code: ~300 bytes

## Debugging

### Echo Received Characters

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        buff[index_uart++] = data_rx;

        // Echo for debugging
        HAL_UART_Transmit(&huart1, &data_rx, 1, 10);

        HAL_UART_Receive_IT(&huart1, &data_rx, 1);
    }
}
```

### Print Buffer Contents

```c
void DebugPrintBuffer(void)
{
    printf("Buffer [%d]: ", index_uart);
    for (uint8_t i = 0; i < index_uart; i++)
    {
        if (buff[i] >= 32 && buff[i] <= 126)
        {
            printf("%c", buff[i]);  // Printable
        }
        else
        {
            printf("[0x%02X]", buff[i]);  // Control character
        }
    }
    printf("\n");
}
```

## Dependencies

### Required

- **STM32 HAL UART**: UART interrupt and transmission
- **command_execute.h**: Command execution engine

### Used By

- **main.c**: Main loop processes commands via UART

## Limitations

### Maximum Command Length

**Hard Limit**: 127 bytes (128 - 1 for null terminator)

Commands exceeding this length will overflow buffer.

### Single Command Processing

Only one command can be buffered at a time. If a new command arrives before the previous is processed, data may be corrupted.

**Solution**: Use ring buffer for multiple commands (advanced).

### No Flow Control

The current implementation does not support hardware or software flow control. If STM32 cannot process commands fast enough, data may be lost.

## Summary

The UART Handler Library provides:

- Interrupt-driven UART reception (115200 baud)
- Command buffering (128-byte buffer)
- Automatic command detection (CR/LF terminators)
- Simple flag-based signaling to main loop
- Low CPU overhead (~0.002% utilization)
- Integration with command execution engine

This library enables reliable command reception from the ESP32 module via UART with minimal CPU impact and straightforward main loop integration.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
