# Ring Buffer Library

Implements a circular FIFO buffer for efficient and non-blocking data handling.

## Overview

The Ring Buffer Library provides a circular FIFO (First-In-First-Out) buffer implementation for efficient data buffering. It is commonly used for UART reception, where incoming data is stored by interrupt handlers and processed by the main loop.

## Files

- **ring_buffer.c**: Ring buffer implementation
- **ring_buffer.h**: Ring buffer API and data structure

## Core Data Structure

### Ring Buffer Structure

```c
typedef struct {
    uint8_t buffer[RING_BUFFER_SIZE];  // Data storage array
    volatile uint16_t head;            // Write position (ISR-safe)
    volatile uint16_t tail;            // Read position (ISR-safe)
} ring_buffer_t;
```

**Fields**:

- `buffer[]`: Fixed-size byte array for data storage
- `head`: Write index (incremented when data is added)
- `tail`: Read index (incremented when data is retrieved)

**Volatile Qualifiers**: Ensures ISR and main loop see consistent values.

### Buffer Size

```c
#define RING_BUFFER_SIZE 256
```

**Capacity**: 256 bytes

**Power of 2**: Enables efficient modulo operation using bitwise AND.

## Buffer States

### Empty Buffer

```
head == tail
```

No data available. `RingBuffer_Get()` returns false.

### Partially Filled Buffer

```
head != tail
Available bytes = (head - tail) % RING_BUFFER_SIZE
```

### Full Buffer

```
(head + 1) % RING_BUFFER_SIZE == tail
```

Cannot add more data. `RingBuffer_Put()` returns false.

**Important**: One byte is always left unused to distinguish full from empty.

**Effective Capacity**: 255 bytes (RING_BUFFER_SIZE - 1)

## API Functions

### Initialize Buffer

```c
void RingBuffer_Init(ring_buffer_t *rb);
```

Initializes a ring buffer to empty state.

**Parameters**:

- `rb`: Pointer to ring buffer structure

**Operations**:

- Sets `head = 0`
- Sets `tail = 0`

**Usage Example**:

```c
ring_buffer_t uart_rx_buffer;
RingBuffer_Init(&uart_rx_buffer);
```

**Important**: Must be called before using buffer.

### Put Data (Write)

```c
bool RingBuffer_Put(ring_buffer_t *rb, uint8_t data);
```

Adds one byte to the buffer.

**Parameters**:

- `rb`: Pointer to ring buffer
- `data`: Byte to store

**Returns**:

- `true`: Data successfully added
- `false`: Buffer full, data not added

**Usage Example**:

```c
// ISR: Store incoming UART byte
void USART1_IRQHandler(void)
{
    if (/* RX interrupt */)
    {
        uint8_t byte = USART1->DR;

        if (!RingBuffer_Put(&uart_rx_buffer, byte))
        {
            // Buffer overflow - data lost
            overflow_count++;
        }
    }
}
```

**Behavior**:

1. Check if buffer full: `(head + 1) % SIZE == tail`
2. If not full: `buffer[head] = data`, `head = (head + 1) % SIZE`
3. If full: Return false without modifying buffer

### Get Data (Read)

```c
bool RingBuffer_Get(ring_buffer_t *rb, uint8_t *data);
```

Retrieves one byte from the buffer.

**Parameters**:

- `rb`: Pointer to ring buffer
- `data`: Pointer to store retrieved byte

**Returns**:

- `true`: Data successfully retrieved
- `false`: Buffer empty, no data available

**Usage Example**:

```c
// Main loop: Process received data
uint8_t byte;
while (RingBuffer_Get(&uart_rx_buffer, &byte))
{
    ProcessByte(byte);
}
```

**Behavior**:

1. Check if buffer empty: `head == tail`
2. If not empty: `*data = buffer[tail]`, `tail = (tail + 1) % SIZE`
3. If empty: Return false without modifying data

### Get Available Bytes

```c
uint16_t RingBuffer_Available(ring_buffer_t *rb);
```

Returns the number of bytes currently stored in the buffer.

**Parameters**:

- `rb`: Pointer to ring buffer

**Returns**: Number of available bytes (0 to RING_BUFFER_SIZE - 1)

**Usage Example**:

```c
if (RingBuffer_Available(&uart_rx_buffer) > 0)
{
    printf("Data available: %u bytes\n", RingBuffer_Available(&uart_rx_buffer));
}
```

**Calculation**:

```c
return (RING_BUFFER_SIZE + rb->head - rb->tail) % RING_BUFFER_SIZE;
```

### Get Free Space

```c
uint16_t RingBuffer_Free(ring_buffer_t *rb);
```

Returns the number of free bytes in the buffer.

**Parameters**:

- `rb`: Pointer to ring buffer

**Returns**: Number of free bytes (0 to RING_BUFFER_SIZE - 1)

**Usage Example**:

```c
if (RingBuffer_Free(&uart_rx_buffer) < 10)
{
    printf("Warning: Buffer almost full\n");
}
```

**Calculation**:

```c
return (RING_BUFFER_SIZE - 1) - RingBuffer_Available(rb);
```

### Clear Buffer

```c
void RingBuffer_Clear(ring_buffer_t *rb);
```

Clears all data from the buffer.

**Parameters**:

- `rb`: Pointer to ring buffer

**Operations**:

- Sets `tail = head`
- Does not erase actual data (optimization)

**Usage Example**:

```c
// Discard all pending data
RingBuffer_Clear(&uart_rx_buffer);
```

**Use Cases**:

- Error recovery
- Protocol restart
- Buffer overflow handling

## Circular Indexing

### Modulo Operation

The buffer uses modulo arithmetic to wrap around:

```c
next_index = (current_index + 1) % RING_BUFFER_SIZE;
```

**Example** (SIZE = 256):

```
Index 255 → (255 + 1) % 256 = 0 (wraps to start)
Index 0   → (0 + 1) % 256 = 1
Index 128 → (128 + 1) % 256 = 129
```

### Power-of-2 Optimization

Since `RING_BUFFER_SIZE = 256 = 2^8`, modulo can be optimized:

```c
next_index = (current_index + 1) & 0xFF;  // Faster than modulo
```

**Benefit**: Bitwise AND is faster than division-based modulo.

## Thread Safety

### ISR-Safe Operations

The ring buffer is designed for ISR/main loop communication:

**Typical Usage Pattern**:

- **ISR**: Calls `RingBuffer_Put()` (write operation)
- **Main Loop**: Calls `RingBuffer_Get()` (read operation)

**Volatile Qualifiers**: Ensure visibility across contexts.

### Single Producer, Single Consumer

**Safe Scenario**:

```c
// ISR: Producer (writes only)
void USART_IRQHandler(void)
{
    RingBuffer_Put(&buffer, data);  // Safe: Only ISR writes
}

// Main loop: Consumer (reads only)
void main(void)
{
    RingBuffer_Get(&buffer, &data);  // Safe: Only main loop reads
}
```

**Unsafe Scenario**:

```c
// Multiple writers (ISR + main loop)
void USART_IRQHandler(void)
{
    RingBuffer_Put(&buffer, data);  // UNSAFE: Both ISR and main loop write
}

void main(void)
{
    RingBuffer_Put(&buffer, other_data);  // UNSAFE
}
```

**Solution**: Use critical sections or separate buffers.

### Critical Sections (if needed)

For multi-writer scenarios:

```c
void SafePut(ring_buffer_t *rb, uint8_t data)
{
    __disable_irq();  // Enter critical section
    RingBuffer_Put(rb, data);
    __enable_irq();   // Exit critical section
}
```

## Usage Examples

### UART Reception

```c
// Global buffer
ring_buffer_t uart_rx_buffer;

// Initialization
void UART_Init(void)
{
    RingBuffer_Init(&uart_rx_buffer);
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);  // Start reception
}

// ISR callback
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        RingBuffer_Put(&uart_rx_buffer, rx_byte);
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);  // Re-enable reception
    }
}

// Main loop processing
void ProcessUART(void)
{
    uint8_t byte;
    while (RingBuffer_Get(&uart_rx_buffer, &byte))
    {
        // Process received byte
        if (byte == '\n')
        {
            ProcessCommand();
        }
        else
        {
            command_buffer[cmd_index++] = byte;
        }
    }
}
```

### Command Line Buffer

```c
char cmd_buffer[128];
uint8_t cmd_index = 0;

void BuildCommand(void)
{
    uint8_t ch;
    while (RingBuffer_Get(&uart_rx_buffer, &ch))
    {
        if (ch == '\r' || ch == '\n')
        {
            // Complete command received
            cmd_buffer[cmd_index] = '\0';
            COMMAND_EXECUTE(cmd_buffer);
            cmd_index = 0;
        }
        else if (ch == '\b' || ch == 0x7F)  // Backspace
        {
            if (cmd_index > 0)
                cmd_index--;
        }
        else if (cmd_index < sizeof(cmd_buffer) - 1)
        {
            cmd_buffer[cmd_index++] = ch;
        }
    }
}
```

### Buffer Monitoring

```c
void MonitorBuffer(void)
{
    uint16_t available = RingBuffer_Available(&uart_rx_buffer);
    uint16_t free = RingBuffer_Free(&uart_rx_buffer);

    printf("Buffer status: %u/%u bytes used (%.1f%%)\n",
           available, RING_BUFFER_SIZE - 1,
           (float)available / (RING_BUFFER_SIZE - 1) * 100.0);

    if (free < 10)
    {
        printf("WARNING: Buffer nearly full!\n");
    }
}
```

### Overflow Detection

```c
uint32_t overflow_count = 0;

void USART_RX_Handler(void)
{
    uint8_t byte = USART->DR;

    if (!RingBuffer_Put(&uart_rx_buffer, byte))
    {
        overflow_count++;

        if (overflow_count % 100 == 0)
        {
            printf("ERROR: %lu buffer overflows\n", overflow_count);
        }
    }
}
```

## Performance Characteristics

### Time Complexity

| Operation            | Time Complexity |
| -------------------- | --------------- |
| RingBuffer_Put       | O(1)            |
| RingBuffer_Get       | O(1)            |
| RingBuffer_Available | O(1)            |
| RingBuffer_Free      | O(1)            |
| RingBuffer_Clear     | O(1)            |

All operations are constant time.

### Execution Time (STM32F103 @ 72 MHz)

| Operation            | Execution Time |
| -------------------- | -------------- |
| RingBuffer_Put       | ~0.5 µs        |
| RingBuffer_Get       | ~0.5 µs        |
| RingBuffer_Available | ~0.3 µs        |
| RingBuffer_Clear     | ~0.1 µs        |

**ISR Overhead**: Negligible (<1 µs per byte).

## Memory Footprint

### RAM Usage (per buffer instance)

```
Structure size: 260 bytes
  - buffer[256]: 256 bytes
  - head: 2 bytes
  - tail: 2 bytes
```

### Flash Usage

- Function code: ~300 bytes

## Buffer Size Considerations

### Current Size (256 bytes)

**Advantages**:

- Sufficient for typical UART command reception
- Power-of-2 enables optimization
- Reasonable RAM usage

**Limitations**:

- Cannot buffer more than 255 bytes
- Full at 115200 baud in ~22ms of continuous data

### Changing Buffer Size

To increase buffer size:

```c
#define RING_BUFFER_SIZE 512  // Must be power of 2
```

**Valid Sizes**: 128, 256, 512, 1024, 2048, etc.

**Trade-off**: Larger buffer uses more RAM.

### Size Calculation for UART

```
Bytes per second = Baud Rate / 10
Buffer time (ms) = (RING_BUFFER_SIZE / Bytes per second) * 1000

Example @ 115200 baud:
Bytes per second = 115200 / 10 = 11520
Buffer time = (256 / 11520) * 1000 = 22.2 ms
```

## Error Handling

### Overflow Detection

```c
uint32_t rx_count = 0;
uint32_t overflow_count = 0;

void HandleRX(uint8_t byte)
{
    rx_count++;

    if (!RingBuffer_Put(&uart_rx_buffer, byte))
    {
        overflow_count++;
    }
}

void PrintStatistics(void)
{
    float overflow_rate = (float)overflow_count / rx_count * 100.0;
    printf("RX: %lu bytes, Overflow: %lu (%.2f%%)\n",
           rx_count, overflow_count, overflow_rate);
}
```

### Recovery Strategies

**Strategy 1: Drop oldest data**

```c
if (!RingBuffer_Put(&buffer, byte))
{
    // Buffer full, remove one byte to make space
    uint8_t dummy;
    RingBuffer_Get(&buffer, &dummy);
    RingBuffer_Put(&buffer, byte);  // Now succeeds
}
```

**Strategy 2: Clear buffer on overflow**

```c
if (!RingBuffer_Put(&buffer, byte))
{
    RingBuffer_Clear(&buffer);  // Discard all pending data
    RingBuffer_Put(&buffer, byte);
}
```

**Strategy 3: Log and continue**

```c
if (!RingBuffer_Put(&buffer, byte))
{
    log_error("Buffer overflow at %lu ms", HAL_GetTick());
    // Continue without storing byte
}
```

## Best Practices

### 1. Initialize Before Use

```c
ring_buffer_t buffer;
RingBuffer_Init(&buffer);  // Always initialize
```

### 2. Check Return Values

```c
// Good: Check if put succeeded
if (!RingBuffer_Put(&buffer, data))
{
    // Handle overflow
}

// Bad: Ignore return value
RingBuffer_Put(&buffer, data);  // May fail silently
```

### 3. Process Data Regularly

```c
// Good: Process in main loop
while (1)
{
    ProcessRingBuffer();  // Prevent buffer overflow
}

// Bad: Infrequent processing
while (1)
{
    HAL_Delay(1000);
    ProcessRingBuffer();  // Buffer may overflow during delay
}
```

### 4. Monitor Buffer Usage

```c
// Periodic buffer health check
if (RingBuffer_Available(&buffer) > (RING_BUFFER_SIZE * 0.8))
{
    printf("WARNING: Buffer 80%% full\n");
}
```

## Limitations

### Maximum Data Size

- **Fixed Size**: Cannot dynamically resize
- **One Byte Loss**: Effective capacity is 255 bytes (not 256)
- **No Peeking**: Cannot read data without removing it

### Not Multi-Reader/Multi-Writer Safe

The current implementation assumes:

- Single producer (e.g., UART ISR)
- Single consumer (e.g., main loop)

For multiple producers or consumers, use locks or separate buffers.

## Dependencies

### Required

- **stdint.h**: Integer types (uint8_t, uint16_t)
- **stdbool.h**: Boolean type

### Used By

- **uart.c**: UART reception buffering
- **Any module needing FIFO buffering**

## Summary

The Ring Buffer Library provides:

- Efficient circular FIFO buffer (256 bytes)
- ISR-safe single-producer, single-consumer operation
- O(1) constant-time operations
- Minimal memory footprint (260 bytes RAM)
- Overflow detection support
- Easy integration with UART and other data streams

This library is essential for reliable UART communication and any scenario requiring temporary data buffering with predictable performance.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
