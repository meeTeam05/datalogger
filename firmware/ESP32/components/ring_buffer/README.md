# Ring Buffer Component

This component provides a circular FIFO (First In First Out) buffer implementation optimized for ESP32 interrupt service routine (ISR) safety. The ring buffer is used primarily for UART data reception in the DATALOGGER system.

## Component Files

```
ring_buffer/
├── ring_buffer.h          # Public API header with structure and function declarations
├── ring_buffer.c          # Implementation of ring buffer operations
├── CMakeLists.txt         # ESP-IDF build configuration
├── component.mk           # Legacy build system support
└── README.md              # This file
```

## Overview

The ring buffer implements a fixed-size circular buffer with 256-byte capacity. It uses volatile head and tail pointers to ensure safe operation in interrupt contexts, making it suitable for UART reception where data arrives asynchronously via hardware interrupts.

## Key Features

- Fixed 256-byte capacity (defined by RING_BUFFER_SIZE macro)
- Thread-safe and ISR-safe operations using volatile pointers
- Overflow protection with return status indication
- FIFO ordering guarantees data sequence preservation
- Buffer status monitoring functions for available and free space
- Clear function for buffer reset

## Data Structure

```c
typedef struct {
    uint8_t buffer[RING_BUFFER_SIZE];  // Data storage array (256 bytes)
    volatile uint16_t head;             // Write position (updated by ISR)
    volatile uint16_t tail;             // Read position (updated by main loop)
} ring_buffer_t;
```

The volatile qualifier on head and tail prevents compiler optimization that could cause data corruption in ISR contexts.

## API Functions

### Initialization

**RingBuffer_Init**

```c
void RingBuffer_Init(ring_buffer_t *rb);
```

Initializes the ring buffer structure by setting head and tail pointers to zero. Must be called before any other ring buffer operations.

Parameters:

- rb: Pointer to ring buffer structure to initialize

### Data Operations

**RingBuffer_Put**

```c
bool RingBuffer_Put(ring_buffer_t *rb, uint8_t data);
```

Adds a single byte to the ring buffer at the head position.

Parameters:

- rb: Pointer to ring buffer structure
- data: Byte value to store in buffer

Returns:

- true: Data successfully added to buffer
- false: Buffer is full, data not added

**RingBuffer_Get**

```c
bool RingBuffer_Get(ring_buffer_t *rb, uint8_t *data);
```

Retrieves a single byte from the ring buffer at the tail position.

Parameters:

- rb: Pointer to ring buffer structure
- data: Pointer to store the retrieved byte

Returns:

- true: Data successfully retrieved from buffer
- false: Buffer is empty, no data available

### Status Functions

**RingBuffer_Available**

```c
uint16_t RingBuffer_Available(ring_buffer_t *rb);
```

Returns the number of bytes currently stored in the buffer and available for reading.

Parameters:

- rb: Pointer to ring buffer structure

Returns: Number of bytes available (0 to RING_BUFFER_SIZE)

**RingBuffer_Free**

```c
uint16_t RingBuffer_Free(ring_buffer_t *rb);
```

Returns the number of free bytes available for writing.

Parameters:

- rb: Pointer to ring buffer structure

Returns: Number of free bytes (0 to RING_BUFFER_SIZE)

**RingBuffer_Clear**

```c
void RingBuffer_Clear(ring_buffer_t *rb);
```

Resets the ring buffer by setting head and tail pointers to zero, effectively discarding all stored data.

Parameters:

- rb: Pointer to ring buffer structure

## Usage Example

```c
#include "ring_buffer.h"

// Declare ring buffer structure
ring_buffer_t uart_rx_buffer;

void app_main(void) {
    // Initialize the ring buffer
    RingBuffer_Init(&uart_rx_buffer);

    // UART ISR adds data
    // RingBuffer_Put(&uart_rx_buffer, received_byte);

    // Main loop reads data
    while (RingBuffer_Available(&uart_rx_buffer) > 0) {
        uint8_t byte;
        if (RingBuffer_Get(&uart_rx_buffer, &byte)) {
            // Process byte
        }
    }
}
```

## Integration with UART

This component is used by the stm32_uart component to buffer incoming UART data:

1. UART hardware receives byte and triggers interrupt
2. ISR calls RingBuffer_Put() to store byte
3. Main loop calls RingBuffer_Available() to check for data
4. Main loop calls RingBuffer_Get() to retrieve and process bytes

## Thread Safety

The ring buffer is designed for single-producer single-consumer pattern:

- One producer (ISR) writes data using RingBuffer_Put()
- One consumer (main loop) reads data using RingBuffer_Get()

This pattern with volatile pointers ensures thread safety without requiring mutex locks.

## Performance Characteristics

- Put operation: O(1) constant time
- Get operation: O(1) constant time
- Available/Free check: O(1) constant time
- Memory usage: 256 bytes data storage + 4 bytes metadata = 260 bytes total

## Limitations

- Fixed 256-byte capacity (not dynamically resizable)
- Byte-oriented storage only (no multi-byte data types)
- Overflow causes data loss (returns false but does not expand)
- Not suitable for multi-producer or multi-consumer scenarios

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
