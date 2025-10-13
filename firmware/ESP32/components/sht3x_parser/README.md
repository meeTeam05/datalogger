# SHT3X Parser Component

Data parser for SHT3X temperature and humidity sensor communication from STM32.

## Features
- Parse SHT3X sensor data strings
- Support for single and periodic measurement modes
- Temperature and humidity extraction
- Data validation and error handling
- Callback system for different measurement types

## Data Format

Parses strings from STM32 in format:
- `"SINGLE 25.30 65.40"` - Single measurement
- `"PERIODIC 26.15 68.20"` - Periodic measurement

## Usage

1. Initialize parser with callbacks for single/periodic data
2. Parse incoming data lines from STM32
3. Handle temperature/humidity values through callbacks
4. Validate data integrity

## Key Functions

- `SHT3X_Parser_Init()` - Initialize with callbacks
- `SHT3X_Parser_ParseLine()` - Parse data string
- `SHT3X_Parser_ProcessLine()` - Parse and trigger callbacks
- `SHT3X_Parser_IsValid()` - Validate parsed data

## License

MIT License - see project root for details.
