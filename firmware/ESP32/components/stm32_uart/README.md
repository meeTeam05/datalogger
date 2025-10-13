# STM32 UART Component

UART communication library for ESP32 to STM32 data exchange with buffering and line processing.

## Features
- Configurable UART parameters (pins, baud rate)
- Ring buffer for received data
- Line-based message processing
- Command sending capability
- Background task for data processing
- Callback system for incoming messages

## Configuration

Configure UART parameters:
- UART port number
- Baud rate
- TX/RX GPIO pins
- Data callback handler

## Usage

1. Initialize UART with pins and baud rate
2. Register callback for incoming line data
3. Start processing task
4. Send commands to STM32
5. Receive processed lines through callback

## Key Functions

- `STM32_UART_Init()` - Initialize UART communication
- `STM32_UART_SendCommand()` - Send command to STM32
- `STM32_UART_StartTask()` - Start data processing task
- `STM32_UART_ProcessData()` - Process buffered data

## License

MIT License - see project root for details.
