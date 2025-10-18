# Relay Control Component

Simple relay control library for ESP32 with state management and command parsing.

## Features
- GPIO-based relay control
- State callback notifications
- Command string parsing ("ON", "OFF", "1", "0")
- State persistence and validation

## Configuration

Configure for your relay:
- GPIO pin number
- Initial state
- State change callback (optional)

## Usage

1. Initialize relay with GPIO pin
2. Control relay state directly or via commands
3. Monitor state changes through callback
4. Toggle state as needed

## Key Functions

- `Relay_Init()` - Initialize relay with GPIO
- `Relay_DeInit()` - DeInitialize relay with GPIO
- `Relay_SetState()` - Control relay (ON/OFF)
- `Relay_Toggle()` - Toggle current state
- `Relay_ProcessCommand()` - Parse and execute commands

## License

MIT License - see project root for details.
