# Ring Buffer Component

Circular FIFO buffer implementation for ESP32 with ISR-safe operations.

## Features
- Fixed-size circular buffer (256 bytes)
- Thread and ISR safe operations
- Overflow protection
- FIFO (First In First Out) behavior
- Buffer status monitoring

## Usage

1. Initialize ring buffer structure
2. Put data into buffer (returns false if full)
3. Get data from buffer (returns false if empty)
4. Monitor available/free space

## Key Functions

- `RingBuffer_Init()` - Initialize buffer
- `RingBuffer_Put()` - Add byte to buffer
- `RingBuffer_Get()` - Read byte from buffer
- `RingBuffer_Available()` - Get bytes available
- `RingBuffer_Free()` - Get free space

## License

MIT License - see project root for details.
